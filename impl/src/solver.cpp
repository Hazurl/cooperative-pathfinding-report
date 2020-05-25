#include <errno.h>

#include <signal.h>
#include <sys/resource.h>

#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include <glucose-syrup-4.1/core/Dimacs.h>
#include <glucose-syrup-4.1/simp/SimpSolver.h>
#include <glucose-syrup-4.1/utils/Options.h>
#include <glucose-syrup-4.1/utils/ParseUtils.h>
#include <glucose-syrup-4.1/utils/System.h>

#include <cpf/Agent.hpp>
#include <cpf/Clause.hpp>
#include <cpf/CmdArg.hpp>
#include <cpf/Context.hpp>
#include <cpf/FileSerializer.hpp>
#include <cpf/Graph.hpp>
#include <cpf/MDD.hpp>
#include <cpf/Variable.hpp>

//=================================================================================================
Glucose::Solver* current_global_solver = nullptr;
bool interrupted					   = false;
// Terminate by notifying the solver and back out gracefully. This is mainly to have a test-case
// for this feature of the Solver as it may take longer than an immediate call to '_exit()'.
void SIGINT_interrupt(int) {
	// Try to interrupt softly the solver
	std::cout << "Interrupt!\n";
	interrupted = true;
	if (current_global_solver) {
		std::cout << "Interrupt Solver!\n";
		current_global_solver->interrupt();
	}
}


void set_cpu_limit(int lim) {
	// Set limits in seconds to the cpu
	// The hard limit, e.g. the app is killed after twice the amount of seconds
	rlimit rl;
	getrlimit(RLIMIT_CPU, &rl);
	if (rl.rlim_max == RLIM_INFINITY || static_cast<rlim_t>(lim) < rl.rlim_max) {
		rl.rlim_cur = lim;
		rl.rlim_max = lim > INT32_MAX / 2 ? INT32_MAX : lim * 2;
		std::cout << "Set cpu limit to " << lim << "s\n";
		if (setrlimit(RLIMIT_CPU, &rl) == -1) {
			std::cerr << "Could not set resource limit: CPU-time.\n";
		}
	}
}

void init_glucose(int cpu_lim) {
#if defined(__linux__)
	fpu_control_t oldcw, newcw;
	_FPU_GETCW(oldcw);
	newcw = (oldcw & ~_FPU_EXTENDED) | _FPU_DOUBLE;
	_FPU_SETCW(newcw);
	// printf("c WARNING: for repeatability, setting FPU to use double precision\n");
#endif
	signal(SIGINT, SIGINT_interrupt);
	signal(SIGXCPU, SIGINT_interrupt);

	if (cpu_lim != INT32_MAX) {
		set_cpu_limit(cpu_lim);
	}
}

bool solve(cpf::Context const& context, std::vector<bool>& res) {
	// Setup glucose's SAT solver
	Glucose::SimpSolver solver;

	solver.parsing			  = 1;
	solver.use_simplification = true;

	solver.verbosity		  = -1;
	solver.verbEveryConflicts = 10000;
	solver.showModel		  = true;

	solver.certifiedUNSAT = false;
	solver.vbyte		  = false;

	for (auto const& clause : context) {
		Glucose::vec<Glucose::Lit> glucose_clause;
		glucose_clause.capacity(clause.variables.size());
		for (auto const& var : clause.variables) {
			while (var.id >= solver.nVars()) { solver.newVar(); }

			// Push each clauses
			glucose_clause.push(Glucose::mkLit(var.id, var.negated));

			if (interrupted)
				return false;
		}
		solver.addClause(glucose_clause);
	}

	solver.parsing = 0;
	solver.eliminate(true);
	if (!solver.okay()) {
		return false;
	}

	if (interrupted)
		return false;

	current_global_solver = &solver;
	bool ret			  = solver.solve();
	if (!ret) {
		return false;
	}

	current_global_solver = nullptr;

	auto const nvars = static_cast<std::size_t>(solver.nVars());
	std::vector<bool> values(nvars);
	for (std::size_t i = 0; i < nvars; ++i) { values[i] = solver.model[i] == l_True; }

	res = std::move(values);
	return true;
}

void print_help(char const* prog_name) {
	std::cerr << "Usage: " << prog_name << " <options> --input=<file>\n";
	std::cerr << "\t--input=<file>         File in CPF format [REQUIRED]\n";
	std::cerr << "Options:\n";
	std::cerr << "\t--min-makespan=<value> Minimum makespan researched\n";
	std::cerr << "\t--max-makespan=<value> Maximum makespan researched\n";
	std::cerr << "\t--max-time=<value>     Maximum amount of seconds to solve the CPF\n";
	std::cerr << "\t--trust                Don't verify that a solution exists (Doesn't do anything)\n";
	std::cerr << "\t--no-mdd               Don't reduce search space\n";
	std::cerr << "\t--output=<file>        Write path of all agents to <file>, each line is a path, each path is a "
				 "sequence of number representing nodes\n";
}

int get_max_cpu_from_args(cpf::CmdArgMap const& args) {
	long o;
	if (cpf::get_argument_as_long(args, "max-time", o)) {
		return static_cast<int>(o);
	} else {
		return INT32_MAX;
	}
}

int get_min_makespan(cpf::CmdArgMap const& args) {
	long o;
	if (cpf::get_argument_as_long(args, "min-makespan", o)) {
		return static_cast<int>(o < 0l ? 0l : o);
	} else {
		return 0;
	}
}

int get_max_makespan(cpf::CmdArgMap const& args) {
	long o;
	if (cpf::get_argument_as_long(args, "max-makespan", o)) {
		return static_cast<int>(o < 0l ? 0l : o);
	} else {
		return INT32_MAX;
	}
}

bool build_context(
	cpf::Context& context,
	cpf::Graph const& graph,
	std::vector<cpf::Agent> const& agents,
	std::size_t makespan,
	bool use_mdd) {
	context = cpf::Context(makespan, agents.size(), graph.size());

	// Create the context, and fill it with the clauses

	std::vector<cpf::MDD> mdds;
	if (use_mdd) {
		mdds.reserve(agents.size());
		for (auto const& agent : agents) { mdds.emplace_back(graph, agent); }

		for (auto& mdd : mdds) { mdd.step_until(makespan); }
	}

	for (std::size_t a = 0; a < agents.size(); ++a) {
		bool has_variable = false;
		for (std::size_t t = 0; t <= makespan; ++t) {
			for (std::size_t v = 0; v < graph.size(); ++v) {
				if (!use_mdd || mdds[a].accessible(v, t, makespan)) {
					context.create_var(t, a, v);
					has_variable = true;
				}
			}
		}

		// If an agent has no path, no solution could exist, early exit
		if (!has_variable) {
			std::cout << "\tNo path for agent " << a << " found in the MDD\n";
			return false;
		}
	}

	// Clause #1
	// !X(t, a, v) or X(t+1, a, v) or OR(u, u -> v exists) X(t+1, a, u)
	for (std::size_t a = 0; a < agents.size(); ++a) {
		for (std::size_t t = 0; t < makespan; ++t) {
			for (std::size_t v = 0; v < graph.size(); ++v) {
				if (context.contains(t, a, v)) {
					auto x0			   = !context.get_var(t, a, v);
					cpf::Clause clause = x0;
					for (std::size_t u = 0; u < graph.size(); ++u) {
						if (u == v || graph[{ v, u }]) {
							if (context.contains(t + 1, a, u)) {
								clause |= context.get_var(t + 1, a, u);
							}
						}
					}
					context.push(clause);
				}
			}
		}
	}

	// Clause #2
	// !X(t, a, v) or !X(t, b, v)
	for (std::size_t a = 0; a < agents.size(); ++a) {
		for (std::size_t b = 0; b < agents.size(); ++b) {
			if (b == a)
				continue;

			for (std::size_t t = 0; t <= makespan; ++t) {
				for (std::size_t v = 0; v < graph.size(); ++v) {
					if (context.contains(t, a, v) && context.contains(t, b, v)) {
						auto x0 = !context.get_var(t, a, v);
						auto x1 = !context.get_var(t, b, v);
						context.push(x0 | x1);
					}
				}
			}
		}
	}

	// Clause #3
	// !X(t, a, v) or !X(t, a, u)
	for (std::size_t a = 0; a < agents.size(); ++a) {
		for (std::size_t t = 0; t <= makespan; ++t) {
			for (std::size_t v = 0; v < graph.size(); ++v) {
				for (std::size_t u = 0; u < graph.size(); ++u) {
					if (u == v)
						continue;
					if (context.contains(t, a, v) && context.contains(t, a, u)) {
						auto x0 = !context.get_var(t, a, v);
						auto x1 = !context.get_var(t, a, u);
						context.push(x0 | x1);
					}
				}
			}
		}
	}

	// Clause #4
	// !X(t, a, v) or !X(t+1, a, u) or !X(t, b, u) or !X(t+1, b, v)
	for (std::size_t a = 0; a < agents.size(); ++a) {
		for (std::size_t b = 0; b < agents.size(); ++b) {
			if (a == b)
				continue;

			for (std::size_t t = 0; t < makespan; ++t) {
				for (std::size_t v = 0; v < graph.size(); ++v) {
					for (std::size_t u = 0; u < graph.size(); ++u) {
						if (u == v)
							continue;
						if (!graph[{ v, u }])
							continue;

						if (context.contains(t, a, v) && context.contains(t + 1, a, u) && context.contains(t, b, u)
							&& context.contains(t + 1, b, v)) {
							auto x0 = !context.get_var(t, a, v);
							auto x1 = !context.get_var(t + 1, a, u);
							auto x2 = !context.get_var(t, b, u);
							auto x3 = !context.get_var(t + 1, b, v);
							context.push(x0 | x1 | x2 | x3);
						}
					}
				}
			}
		}
	}

	std::vector<std::size_t> initial_nodes_with_agents(graph.size(), agents.size());
	std::vector<std::size_t> goal_nodes_with_agents(graph.size(), agents.size());

	for (std::size_t a = 0; a < agents.size(); ++a) {
		auto const& agent = agents[a];

		if (initial_nodes_with_agents.at(agent.initial) < agents.size()) {
			throw std::runtime_error("Agent already exists at node " + std::to_string(agent.initial));
		}
		initial_nodes_with_agents[agent.initial] = a;

		if (goal_nodes_with_agents.at(agent.goal) < agents.size()) {
			throw std::runtime_error("Agent already exists at node " + std::to_string(agent.goal));
		}
		goal_nodes_with_agents[agent.goal] = a;
	}

	// Init
	for (std::size_t a = 0; a < agents.size(); ++a) {
		for (std::size_t v = 0; v < graph.size(); ++v) {
			if (context.contains(0, a, v)) {
				auto x = context.get_var(0, a, v);
				if (initial_nodes_with_agents[v] == a) {
					context.push(x);
				} else {
					context.push(!x);
				}
			}
		}
	}

	// Goal
	for (std::size_t a = 0; a < agents.size(); ++a) {
		for (std::size_t v = 0; v < graph.size(); ++v) {
			if (context.contains(makespan, a, v)) {
				auto x = context.get_var(makespan, a, v);
				if (goal_nodes_with_agents[v] == a) {
					context.push(x);
				} else {
					context.push(!x);
				}
			}
		}
	}

	return true;
}

int main(int argc, char** argv) {
	// Setup args
	auto args = cpf::parse_args(argc, argv);
	if (cpf::has_argument(args, "help") || cpf::has_argument(args, "h")) {
		print_help(argv[0]);
		return 0;
	}

	std::string input_filename;
	if (!cpf::get_argument_as_string(args, "input", input_filename)) {
		std::cerr << "Missing input file\n";
		print_help(argv[0]);
		return 3;
	}

	int max_cpu = get_max_cpu_from_args(args);

	std::pair<int, int> makespan_interval = { get_min_makespan(args), get_max_makespan(args) };
	// bool verify_solution_exists = !cpf::has_argument(args, "trust");
	bool use_mdd = !cpf::has_argument(args, "no-mdd");

	std::ifstream ifile(input_filename);
	if (!ifile) {
		std::cerr << "Unable to read file '" << input_filename << "'\n";
		return 2;
	}
	auto deserialized_data = cpf::deserialize(ifile);
	auto& graph			   = deserialized_data.first;
	auto& agents		   = deserialized_data.second;

	init_glucose(max_cpu);

	cpf::Context context;
	std::vector<bool> res;

	auto clock_all_begin   = std::chrono::steady_clock::now();
	auto report_total_time = [&]() {
		auto clock_all_end								   = std::chrono::steady_clock::now();
		std::chrono::duration<double, std::milli> duration = clock_all_end - clock_all_begin;
		std::cout << "Total time: " << duration.count() << "ms\n";
	};

	// Start iterative methods

	int makespan;
	for (makespan = makespan_interval.first; makespan <= makespan_interval.second && !interrupted; ++makespan) {
		auto clock_begin = std::chrono::steady_clock::now();
		auto report_time = [&]() {
			auto clock_end									   = std::chrono::steady_clock::now();
			std::chrono::duration<double, std::milli> duration = clock_end - clock_begin;
			std::cout << "\tTook " << duration.count() << "ms\n";
		};

		std::cout << "Generating SAT problem with a bounded makespan of " << makespan << "...\n";
		if (!build_context(context, graph, agents, makespan, use_mdd)) {
			report_time();
			continue;
		}

		std::cout << "\t#Variables: " << context.variables_count() << '\n';
		std::cout << "\t#Clauses: " << context.clauses_count() << '\n';

		std::cout << "\tSolving...\n";
		if (interrupted || solve(context, res)) {
			report_time();
			break;
		}

		report_time();
		std::cout << "\tFailed to solve.\n";
	}

	if (interrupted) {
		std::cout << "\tFailed to solve.\n";
		report_total_time();
		std::cout << "No solution found in time\n";
		return 1;
	}

	if (makespan > makespan_interval.second) {
		std::cout << "\tFailed to solve.\n";
		report_total_time();
		std::cout << "No solution found within the bounds\n";
		return 1;
	}

	std::cout << "\tSuccessfully solved\n";
	report_total_time();

	// Display path
	std::cout << "Path of all agents:\n";
	for (std::size_t a = 0; a < agents.size(); ++a) {
		std::cout << "\tAgent #" << a << ": ";
		for (std::size_t t = 0; t <= makespan; ++t) {
			for (std::size_t v = 0; v < graph.size(); ++v) {
				if (context.contains(t, a, v) && res[context.get_var(t, a, v).id]) {
					std::cout << "#" << v << ", ";
				}
			}
		}
		std::cout << '\n';
	}

	// Writing path to file if requested
	std::string output_file;
	if (cpf::get_argument_as_string(args, "output", output_file)) {
		std::ofstream file(output_file);
		if (!file) {
			std::cerr << "Couldn't open output file '" << output_file << "'\n";
			return 1;
		}
		std::cout << "Writing to '" << output_file << "'... ";

		for (std::size_t a = 0; a < agents.size(); ++a) {
			for (std::size_t t = 0; t <= makespan; ++t) {
				for (std::size_t v = 0; v < graph.size(); ++v) {
					if (context.contains(t, a, v) && res[context.get_var(t, a, v).id]) {
						file << v << ' ';
					}
				}
			}
			file << '\n';
		}

		std::cout << "Done\n";
	}

	return 0;
}
