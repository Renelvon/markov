#include <algorithm>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <random>

using std::cout;
using std::endl;

// Single-up, Single-Down Markov state data structure
struct state_t {
    double          probup;         // Probability of next state being *next_up    
    struct state_t  *next_up;       // Next state when a packet arrives
    struct state_t  *next_down;     // Next state when a packet completes service
    int             arrivals_gen;   // Arrivals during current generation
    int             arrivals_tot;   // Total arrivals over all generations
    double          oldp;           // Cache for ergodic probability
};

void usage () {
    cout << "Usage: ./main <N> <lambda> <mu> <K> <e> <maxg> <eps> <v>" << endl
         << "  N     : System capacity [packets]" << endl
         << "  lambda: Mean arrival rate [packets / sec]" << endl
         << "  mu    : Mean queue service rate [packets / sec]" << endl
         << "  K     : Service initiation threshold [packets]" << endl
         << "  e     : Events per simulation generation" << endl
         << "  maxg  : Maximum generations simulated" << endl
         << "  eps   : Tolerance [%]" << endl
         << "  v     : verbose (0|1)" << endl;
}

int main (int argc, char* argv[]) {

    // Parse simulation parameters & convergence criteria.
    int N, K;
    double lambda, mu;

    int events_per_gen, max_gens;
    double eps;
    
    bool verbose;

    if (argc != 9) {
        usage();
        return 1;
    }

    N              = std::max( 1, atoi(argv[1]));
    lambda         = std::fmax(0, atof(argv[2]));
    mu             = std::fmax(0, atof(argv[3]));
    K              = std::max( 0, atoi(argv[4]));
    events_per_gen = std::max( 1, atoi(argv[5]));
    max_gens       =              atoi(argv[6]);
    eps            = std::fmax(0, atof(argv[7]) / 100);
    verbose        =              atoi(argv[8]) != 0 ? true: false;

    // Do not overflow events
    max_gens = std::min(max_gens, std::numeric_limits<int>::max() / events_per_gen - 1);
    
    // Setup a random number generator (Requires C++11)
    std::random_device                  rdev{};       // An entropy source
    std::default_random_engine          eng{rdev()};  // A seeded engine
    std::uniform_real_distribution<>    d(0.0, 1.0);  // A Uniform[0, 1) distr.

    // Setup precision of output stream
    cout << std::setprecision(3);
    cout.setf(std::ios_base::fixed);

    cout << "QUEUEING SYSTEMS SIMULATION 2013" << endl
         << "=================================" << endl
         << endl
         << "Model:" << endl
         << "------" << endl
         << "\tMarkovian A/V streaming server with threshold" << endl
         << endl
         << "Model parametres:" << endl
         << "-----------------" << endl
         << "\tArrivals   : lambda = " << lambda << " packets/sec" << endl
         << "\tService    : mu = " << mu << " packets/sec" << endl
         << "\tQueue limit: N = " << N << " packets" << endl
         << "\tThreshold  : K = " << K << " packets" << endl 
         << endl;


    cout << "Simulation parametres:" << endl
         << "----------------------" << endl
         << "\tEvents per generation: " << events_per_gen << endl
         << "\tMax generations      : " << max_gens << endl
         << "\tTolerance            : " << eps * 100 << "%" << endl
         << "\tVerbose mode         : " << (verbose ? "ON" : "OFF") << endl
         << endl;

    // Setup and initialize state data structure
    /* Type          States              Arrival -> next    Service -> next 
     * ------------  ------------------  -----------------  ---------------
     * Empty         i = 0               lambda -> N + 1    N/A
     * Ordinary      i = [1 .. N-1]      lambda -> (i + 1)  mu -> (i - 1)
     * Full          i = N               lambda -> N        mu -> (N - 1)
     * Buffering     i = [N+1 .. N+K-2]  lambda -> (i + 1)  N/A
     * Service init  i = N+K-1           lambda -> K        N/A
     */
    int total_states = N + K;
    auto states = new struct state_t[total_states];

    states[0].probup = 1.0;
    states[0].next_up = (K <= 1) ? &states[1] : &states[N + 1];
    states[0].next_down = nullptr;
    states[0].arrivals_tot = 0;
    states[0].oldp = 1.0;

    for (int i = 1; i < N; ++i) {
        states[i].probup = static_cast<double>(lambda) / (lambda + mu);
        states[i].next_up = &states[i + 1];
        states[i].next_down = &states[i - 1];
        states[i].arrivals_tot = 0;
        states[i].oldp = 1.0;
    }

    states[N].probup = static_cast<double>(lambda) / (lambda + mu);
    states[N].next_up = &states[N];
    states[N].next_down = &states[N - 1];
    states[N].arrivals_tot = 0;
    states[N].oldp = 1.0;

    for (int i = N + 1; i < N + K - 1; ++i) {
        states[i].probup = 1.0;
        states[i].next_up = &states[i + 1];
        states[i].next_down = nullptr;
        states[i].arrivals_tot = 0;
        states[i].oldp = 1.0;
    }

    states[N + K - 1].probup = 1.0;
    states[N + K - 1].next_up = &states[K];
    states[N + K - 1].next_down = nullptr;
    states[N + K - 1].arrivals_tot = 0;
    states[N + K - 1].oldp = 1.0;

    cout << "Results:" << endl
         << "--------" << endl
         <<"Gen\tEps [%]\tErgodic probabilities [%]" << endl
         << "\t\tOrdinary states";
    for (int i = 1; i < N + 1; ++i) {
        cout << "\t";
    }
    cout << "Buffering states" << endl;

    cout << "\t";
    for (int i = 0; i < N + 1; ++i) {
        cout << "\tP_o(" << i << ")";
    }
    for (int i = 1; i < K; ++i) {
        cout << "\tP_b(" << i << ")";
    }
    cout << endl;

    // Prepare initial state
    int arrivals_tot = 1; // to avoid divide-by-zero
    ++states[0].arrivals_tot;
    struct state_t *cur = &states[1];

    int generation = 0;
    double conv;

    /* Simulation happens in successive generations, each inheriting
     * the final state of the previous generation.
     * During each generation, a fixed number of events is performed
     * and the distribution of arrivals is measured across all states.
     * The simulation stops if at the end of some generation the arrival
     * percentage of all states has not fluctuated beyond accepted tolerance
     * or if the maximum number of generations has been reached.
     */
    do {
        // Prepare states for new generation
        for (int i = 0; i < total_states; ++i) {
            states[i].arrivals_gen = 0;
        }
        int arrivals_gen = 0;

        // Simulate a generation
        for (int e = 0; e < events_per_gen; ++e) {
            if (cur->probup == 1.0 || d(eng) < (cur->probup)) { // arrival
                ++cur->arrivals_gen;
                ++arrivals_gen;
                cur = cur->next_up;
            } else { // service
                cur = cur->next_down;
            }
        }

        // Test convergence & update states
        arrivals_tot += arrivals_gen;
        conv = 0;
        for (int i = 0; i < total_states; ++i) {
            states[i].arrivals_tot += states[i].arrivals_gen;
            if (states[i].arrivals_tot > 0) {
                double newp = static_cast<double>(states[i].arrivals_tot)
                              / arrivals_tot;

                conv = std::max(
                        conv,
                        std::abs((newp - states[i].oldp) / states[i].oldp)
                       );

                states[i].oldp = newp;
            } else { // Unvisited state signals we are far from convergence
                conv = eps + 1;
            }
        }
        ++generation;

        // Conditionally print results up to now
        if (verbose) {
            cout << generation << "\t" << 100 * conv;
            for (int i = 0; i < N + K; ++i) {
                cout << "\t" << 100 * states[i].oldp;
            }
            cout << endl;
        }
        
    } while (conv >= eps && generation < max_gens);

    cout << endl
         << "Final\t" << 100 * conv;
    for (int i = 0; i < N + K; ++i) {
        cout << "\t" << 100 * states[i].oldp;
    }
    cout << endl;

    // Virtual convergence time
    double tconv = static_cast<double>(arrivals_tot) / lambda;

    cout << endl
         << "Simulation statistics:" << endl
         << "----------------------" << endl
         << "\tGenerations simulated   : " << generation << endl
         << "\tEvents simulated        : " << generation * events_per_gen << endl
         << "\tTotal arrivals          : " << arrivals_tot << " packets" << endl
         << "\tVirtual convergence time: " << tconv << " sec" << endl
         << "\t                          " << tconv / 60 << " min" << endl
         << "\t                          " << tconv / 3600 << " h" << endl
         << "\tResult tolerance        : " << 100 * conv << " %" << endl
         << endl;

    // Print aggregate results
    double pbl = states[N].oldp;
    double meanpackets = 0.0;
    for (int i = 1; i < total_states; ++i) {
        meanpackets += (i > N)
                       ? (i - N) * states[i].oldp   // buffering
                       : i       * states[i].oldp;  // ordinary
    }
    double gamma = lambda * (1 - pbl);
    double meantime = meanpackets / gamma;     // Little's Law

    cout << "Model statistics:" << endl
         << "-----------------" << endl
         << "\tOverflow probability: P_bl = " << 100 * pbl << " %" << endl
         << "\tMean queue size     : E_N = " << meanpackets << " packets"
         << endl
         << "\tThroughput          : gamma = " << gamma << " packets/sec"
         << endl
         << "\tMean packet sojourn : T_d = " << meantime << " sec" << endl;

    // Cleanup
    delete[] states;
}
