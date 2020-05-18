#include <errno.h>

#include <signal.h>
#include <sys/resource.h>

#include <string>
#include <vector>
#include <optional>
#include <iostream>
#include <fstream>
#include <cstring>
#include <chrono>
#include <cassert>

#include <glucose-syrup-4.1/utils/System.h>
#include <glucose-syrup-4.1/utils/ParseUtils.h>
#include <glucose-syrup-4.1/utils/Options.h>
#include <glucose-syrup-4.1/core/Dimacs.h>
#include <glucose-syrup-4.1/simp/SimpSolver.h>

#include <Clause.hpp>
#include <Variable.hpp>
#include <Context.hpp>
#include <Graph.hpp>
#include <Agent.hpp>
#include <FileSerializer.hpp>
#include <CmdArg.hpp>
#include <MDD.hpp>

//=================================================================================================
Glucose::Solver* current_global_solver = nullptr;
bool interrupted = false;
// Terminate by notifying the solver and back out gracefully. This is mainly to have a test-case
// for this feature of the Solver as it may take longer than an immediate call to '_exit()'.
void SIGINT_interrupt(int signum) { 
    std::cout << "Interrupt!\n";
    interrupted = true;
    if (current_global_solver) {
        std::cout << "Interrupt Solver!\n";
        current_global_solver->interrupt(); 
    }
}


void set_cpu_limit(int lim) {
    rlimit rl;
    getrlimit(RLIMIT_CPU, &rl);
    if (rl.rlim_max == RLIM_INFINITY || (rlim_t)lim < rl.rlim_max){
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
    _FPU_GETCW(oldcw); newcw = (oldcw & ~_FPU_EXTENDED) | _FPU_DOUBLE; _FPU_SETCW(newcw);
    //printf("c WARNING: for repeatability, setting FPU to use double precision\n");
#endif
    signal(SIGINT, SIGINT_interrupt);
    signal(SIGXCPU,SIGINT_interrupt);

    if (cpu_lim != INT32_MAX) {
        set_cpu_limit(cpu_lim);
    }
}

bool solve(Context const& context, std::vector<bool>& res) {
    Glucose::SimpSolver solver;

    solver.parsing = 1;
    solver.use_simplification = true;

    solver.verbosity = -1;
    solver.verbEveryConflicts = 10000;
    solver.showModel = true;

    solver.certifiedUNSAT = false;
    solver.vbyte = false;

    for(auto const& clause : context) {
        Glucose::vec<Glucose::Lit> glucose_clause;
        glucose_clause.capacity(clause.variables.size());
        for(auto const& var : clause.variables) {
            while(var.id >= solver.nVars()) {
                solver.newVar();
            }

            glucose_clause.push(Glucose::mkLit(var.id, var.negated));

            if (interrupted) return false;
        }
        solver.addClause(glucose_clause);
    }

    solver.parsing = 0;
    solver.eliminate(true);
    if (!solver.okay()){
        return false;
    }

    if (interrupted) return false;

    current_global_solver = &solver;
    bool ret = solver.solve();
    if (!ret) {
        return false;
    }

    current_global_solver = nullptr;

    std::vector<bool> values(solver.nVars());
    for(std::size_t i = 0; i < solver.nVars(); ++i) {
        values[i] = solver.model[i] == l_True;
    }

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
    std::cerr << "\t--trust                Don't verify that a solution exists (This can be useful as the algorithm used for that is incomplete)\n";
    std::cerr << "\t--no-mdd               Don't reduce search space\n";
    std::cerr << "\t--output=<file>        Write path of all agents to <file>, each line is a path, each path is a sequence of number representing nodes\n";
}

int get_max_cpu_from_args(CmdArgMap const& args) {
    long o;
    if (get_argument_as_long(args, "max-time", o)) {
        return static_cast<int>(o);
    } else {
        return INT32_MAX;
    }
}

int get_min_makespan(CmdArgMap const& args) {
    long o;
    if (get_argument_as_long(args, "min-makespan", o)) {
        return static_cast<int>(o < 0l ? 0l : o);
    } else {
        return 0;
    }
}

int get_max_makespan(CmdArgMap const& args) {
    long o;
    if (get_argument_as_long(args, "max-makespan", o)) {
        return static_cast<int>(o < 0l ? 0l : o);
    } else {
        return INT32_MAX;
    }
}

bool build_context(Context& context, Graph const& graph, std::vector<Agent> const& agents, std::size_t makespan, bool use_mdd) {
    
    context = Context(makespan, agents.size(), graph.size());

    std::vector<MDD> mdds;
    if (use_mdd) {
        mdds.reserve(agents.size());
        for(auto const& agent : agents) {
            mdds.emplace_back(graph, agent);
        }

        for(auto& mdd : mdds) {
            mdd.step_until(makespan);
        }
    }

    for(int a = 0; a < agents.size(); ++a) {
        bool has_variable = false;
        for(int t = 0; t <= makespan; ++t) {
            for(int v = 0; v < graph.size(); ++v) {
                if (!use_mdd || mdds[a].accessible(v, t, makespan)) {
                    context.create_var(t, a, v);
                    has_variable = true;
                }
            }
        }

        if (!has_variable) {
            std::cout << "\tNo path for agent " << a << " found in the MDD\n";
            return false;
        }
    }

    // Clause #1
    // !X(t, a, v) or X(t+1, a, v) or OR(u, u -> v exists) X(t+1, a, u) 
    for(int a = 0; a < agents.size(); ++a) {
        for(int t = 0; t < makespan; ++t) {
            for(int v = 0; v < graph.size(); ++v) {
                if (context.contains(t, a, v)) {
                    auto x0 = !context.get_var(t, a, v); 
                    Clause clause = x0;
                    for(int u = 0; u < graph.size(); ++u) {
                        if (u == v || graph[{v, u}]) {
                            if (context.contains(t+1, a, u)) {
                                clause |= context.get_var(t+1, a, u);
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
    for(int a = 0; a < agents.size(); ++a) {
        for(int b = 0; b < agents.size(); ++b) {
            if (b == a) continue;

            for(int t = 0; t <= makespan; ++t) {
                for(int v = 0; v < graph.size(); ++v) {
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
    for(int a = 0; a < agents.size(); ++a) {
        for(int t = 0; t <= makespan; ++t) {
            for(int v = 0; v < graph.size(); ++v) {
                for(int u = 0; u < graph.size(); ++u) {
                    if (u == v) continue;
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
    for(int a = 0; a < agents.size(); ++a) {
        for(int b = 0; b < agents.size(); ++b) {
            if (a == b) continue;

            for(int t = 0; t < makespan; ++t) {
                for(int v = 0; v < graph.size(); ++v) {
                    for(int u = 0; u < graph.size(); ++u) {
                        if (u == v) continue;
                        if (!graph[{v, u}]) continue;

                        if (context.contains(t, a, v) 
                        && context.contains(t+1, a, u)
                        && context.contains(t, b, u)
                        && context.contains(t+1, b, v)
                        ) {
                            auto x0 = !context.get_var(t, a, v); 
                            auto x1 = !context.get_var(t+1, a, u); 
                            auto x2 = !context.get_var(t, b, u); 
                            auto x3 = !context.get_var(t+1, b, v); 
                            context.push(x0 | x1 | x2 | x3);
                        }
                    }
                }
            }
        }
    }

    std::vector<std::size_t> initial_nodes_with_agents(graph.size(), agents.size());
    std::vector<std::size_t> goal_nodes_with_agents(graph.size(), agents.size());

    for(std::size_t a = 0; a < agents.size(); ++a) {
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
    for(int a = 0; a < agents.size(); ++a) {
        for(int v = 0; v < graph.size(); ++v) {
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
    for(int a = 0; a < agents.size(); ++a) {
        for(int v = 0; v < graph.size(); ++v) {
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

int main(int argc, char** argv)
{
    auto args = parse_args(argc, argv);
    if (has_argument(args, "help") || has_argument(args, "h")) {
        print_help(argv[0]);
        return 0;
    }

    std::string input_filename;
    if (!get_argument_as_string(args, "input", input_filename)) {
        std::cerr << "Missing input file\n";
        print_help(argv[0]);
        return 3;
    }

    int max_cpu = get_max_cpu_from_args(args);

    std::pair<int, int> makespan_interval = { get_min_makespan(args), get_max_makespan(args) };
    bool verify_solution_exists = !has_argument(args, "trust");
    bool use_mdd = !has_argument(args, "no-mdd");

    std::ifstream ifile(input_filename);
    if (!ifile) {
        std::cerr << "Unable to read file '" << input_filename << "'\n";
        return 2;
    }
    auto deserialized_data = deserialize(ifile);
    auto& graph = deserialized_data.first;
    auto& agents = deserialized_data.second;

    init_glucose(max_cpu);

    Context context;
    std::vector<bool> res;

    auto clock_all_begin = std::chrono::steady_clock::now();
    auto report_total_time = [&] () {
        auto clock_all_end = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::milli> duration = clock_all_end - clock_all_begin;
        std::cout << "Total time: " << duration.count() << "ms\n";
    };

    int makespan;
    for(makespan = makespan_interval.first; makespan <= makespan_interval.second && !interrupted; ++makespan) {
        auto clock_begin = std::chrono::steady_clock::now();
        auto report_time = [&] () {
            auto clock_end = std::chrono::steady_clock::now();
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

    std::cout << "Path of all agents:\n";
    for(int a = 0; a < agents.size(); ++a) {
        std::cout << "\tAgent #" << a << ": ";
        for(int t = 0; t <= makespan; ++t) {
            for(int v = 0; v < graph.size(); ++v) {
                if (context.contains(t, a, v) && res[context.get_var(t, a, v).id]) {
                    std::cout << "#" << v << ", ";
                }
            }
        }
        std::cout << '\n';
    }

    std::string output_file;
    if (get_argument_as_string(args, "output", output_file)) {
        std::ofstream file(output_file);
        if (!file) {
            std::cerr << "Couldn't open output file '" << output_file << "'\n";
            return 1;
        }
        std::cout << "Writing to '" << output_file << "'... ";

        for(int a = 0; a < agents.size(); ++a) {
            for(int t = 0; t <= makespan; ++t) {
                for(int v = 0; v < graph.size(); ++v) {
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