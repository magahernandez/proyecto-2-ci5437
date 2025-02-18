// Game of Othello -- Example of main
// Universidad Simon Bolivar, 2012.
// Author: Blai Bonet
// Last Revision: 1/11/16
// Modified by: 

#include <iostream>
#include <limits>
#include "othello_cut.h"
#include "utils.h"

#include <unordered_map>

using namespace std;

unsigned expanded = 0;
unsigned generated = 0;
int tt_threshold = 32; // threshold to save entries in TT
int INFINITY = std::numeric_limits<int>::max();

// Transposition table (it is not necessary to implement TT)
struct stored_info_t {
    int value_;
    int type_;
    enum { EXACT, LOWER, UPPER };
    stored_info_t(int value = -100, int type = LOWER) : value_(value), type_(type) { }
};

struct hash_function_t {
    size_t operator()(const state_t &state) const {
        return state.hash();
    }
};

class hash_table_t : public unordered_map<state_t, stored_info_t, hash_function_t> {
};

hash_table_t TTable[2];

//int maxmin(state_t state, int depth, bool use_tt);
//int minmax(state_t state, int depth, bool use_tt = false);
//int maxmin(state_t state, int depth, bool use_tt = false);
int negamax(state_t state, int depth, int color, bool use_tt = false);
int negamax(state_t state, int depth, int alpha, int beta, int color, bool use_tt = false);
int scout(state_t state, int depth, int color, bool use_tt = false);
int negascout(state_t state, int depth, int alpha, int beta, int color, bool use_tt = false);

int main(int argc, const char **argv) {
    state_t pv[128];
    int npv = 0;
    for( int i = 0; PV[i] != -1; ++i ) ++npv;

    int algorithm = 0;
    if( argc > 1 ) algorithm = atoi(argv[1]);
    bool use_tt = argc > 2;

    // Extract principal variation of the game
    state_t state;
    cout << "Extracting principal variation (PV) with " << npv << " plays ... " << flush;
    for( int i = 0; PV[i] != -1; ++i ) {
        bool player = i % 2 == 0; // black moves first!
        int pos = PV[i];
        pv[npv - i] = state;
        state = state.move(player, pos);
    }
    pv[0] = state;
    cout << "done!" << endl;

#if 0
    // print principal variation
    for( int i = 0; i <= npv; ++i )
        cout << pv[npv - i];
#endif

    // Print name of algorithm
    cout << "Algorithm: ";
    if( algorithm == 1 )
        cout << "Negamax (minmax version)";
    else if( algorithm == 2 )
        cout << "Negamax (alpha-beta version)";
    else if( algorithm == 3 )
        cout << "Scout";
    else if( algorithm == 4 )
        cout << "Negascout";
    cout << (use_tt ? " w/ transposition table" : "") << endl;

    // Run algorithm along PV (bacwards)
    cout << "Moving along PV:" << endl;
    for( int i = 0; i <= npv; ++i ) {
        //cout << "pv[i]: " << pv[i] << "\n";
        int value = 0;
        TTable[0].clear();
        TTable[1].clear();
        float start_time = Utils::read_time_in_seconds();
        expanded = 0;
        generated = 0;
        int color = i % 2 == 1 ? 1 : -1;

        try {
            if( algorithm == 1 ) {
                value = negamax(pv[i], 0, color, use_tt);
            } else if( algorithm == 2 ) {
                value = negamax(pv[i], 0, -200, 200, color, use_tt);
            } else if( algorithm == 3 ) {
                value = color * scout(pv[i], 0, color, use_tt);
            } else if( algorithm == 4 ) {
                value = negascout(pv[i], 0, -200, 200, color, use_tt);
            }
        } catch( const bad_alloc &e ) {
            cout << "size TT[0]: size=" << TTable[0].size() << ", #buckets=" << TTable[0].bucket_count() << endl;
            cout << "size TT[1]: size=" << TTable[1].size() << ", #buckets=" << TTable[1].bucket_count() << endl;
            use_tt = false;
        }

        float elapsed_time = Utils::read_time_in_seconds() - start_time;

        cout << npv + 1 - i << ". " << (color == 1 ? "Black" : "White") << " moves: "
             << "value=" << color * value
             << ", #expanded=" << expanded
             << ", #generated=" << generated
             << ", seconds=" << elapsed_time
             << ", #generated/second=" << generated/elapsed_time
             << endl;
    }

    return 0;
}

/* Negamax without alpha-beta prunning */

int negamax(state_t state, int depth, int color, bool use_tt){

    bool player = (color < 0) ? 0 : 1;
    int alpha = -INFINITY;
    bool move = false;

    if (state.terminal()) {
        return color * state.value();
    }

    ++expanded;

    // foreach child of node
    for (int pos = 0; pos < DIM; pos++) {

        // If at least one stone are flanked 
        if (state.outflank(player, pos)) {
            // Moved
            move = true;
            ++generated;
            alpha = max(alpha, -negamax(state.move(player, pos), depth - 1, -color, use_tt));
        }
    }

    if (!move) {
        ++generated;
        alpha = max(alpha, -negamax(state, depth - 1, -color, use_tt));
    }

    return alpha;
}

/* Negamax with alpha-beta prunning */

int negamax(state_t state, int depth, int alpha, int beta, int color, bool use_tt){

    bool player = (color < 0) ? 0 : 1;
    int score = -INFINITY;
    bool move = false;
    int val;

    if (state.terminal() ) {
        return color * state.value();
    }

    ++expanded;

    // foreach child of node
    for (int pos = 0; pos < DIM; pos++) {

        // If at least one stone are flanked 
        if (state.outflank(player, pos)) {
            // Moved
            move = true;
            ++generated;
            val = -negamax(state.move(player, pos), depth - 1, -beta, -alpha, -color, use_tt);
            score = max(score, val);
            alpha = max(alpha, val);
            if (alpha >= beta) break;
        }
    }

    if (!move) {
        ++generated;
        val = -negamax(state, depth - 1, -beta, -alpha, -color, use_tt);
        score = max(score, val);
    }

    return score;
}

/* TEST */

bool TEST(state_t state, int depth, int score, int color, int condition){

    bool player = (color < 0) ? 0 : 1;
    bool move = false;

    if (state.terminal()){
        if (condition == 0){
            return state.value() > score ? true : false;
        }else{
            return state.value() >= score ? true : false;
        }
    }

    ++expanded;

    // foreach child of node
    for (int pos = 0; pos < DIM; pos++) {

        // If at least one stone are flanked 
        if (state.outflank(player, pos)) {

            // Moved
            move = true;
            ++generated;

            // Max
            if (color == 1 && TEST(state.move(player, pos), depth - 1, score, -color, condition)){
                return true;
            }

            // Min
            if (color == -1 && !TEST(state.move(player, pos), depth - 1, score, -color, condition)){
                return false;
            }
        }
    }

    if (!move) {
        if (color == 1 && TEST(state, depth - 1, score, -color, condition)){
            return true;
        }
        if (color == -1 && !TEST(state, depth - 1, score, -color, condition)){
            return false;
        }
        ++generated;
    }

    return color != 1;
}


/* Scout */

int scout(state_t state, int depth, int color, bool use_tt){

    bool player = (color < 0) ? 0 : 1;
    bool move = false;
    int score = 0;
    bool isFirstChild = true;

    if (state.terminal()) {
        return state.value();
    }

    ++expanded;

    // foreach child of node
    for (int pos = 0; pos < DIM; pos++) {

        // If at least one stone are flanked 
        if (state.outflank(player, pos)) {
            // Moved
            move = true;
            ++generated;
            if (isFirstChild) {
                isFirstChild = false;
                score = scout(state.move(player, pos), depth - 1, -color, use_tt);
            }
            else {

                // Max
                if (color == 1 && TEST(state.move(player, pos), depth - 1, score, -color, use_tt)){
                    score = scout(state.move(player, pos), depth - 1, -color, use_tt);
                }

                // Min
                if (color == -1 && !TEST(state.move(player, pos), depth - 1, score,-color, use_tt)){
                    score = scout(state.move(player, pos), depth - 1, -color, use_tt);
                }
            }
        }
    }

    if (!move) {
        score = scout(state, depth - 1, -color, use_tt);
        ++generated;
    }

    return score;
}

/* Negascout = Negamax with alpha-beta prunning + scout */

int negascout(state_t state, int depth, int alpha, int beta, int color, bool use_tt){
    
    bool player = (color < 0) ? 0 : 1;
    bool move = false;
    int score = -INFINITY;
    bool isFirstChild = true;

    if (state.terminal()) {
        return color * state.value();
    }

    ++expanded;

    // foreach child of node
    for (int pos = 0; pos < DIM; pos++) {
        
        // If at least one stone are flanked 
        if (state.outflank(player, pos)) {
            // Moved
            move = true;
            ++generated;
            if (isFirstChild) {
                isFirstChild = false;
                score = -negascout(state.move(player, pos), depth + 1, -beta, -alpha, -color, use_tt);
            }
            else {
                score = -negascout(state.move(player, pos), depth + 1, -alpha - 1, -alpha, -color, use_tt);
                if (alpha<score && score<beta) {
                    score = -negascout(state.move(player, pos), depth + 1, -beta, -score, -color, use_tt);
                }
            }
            alpha = max(alpha, score);
            if (alpha >= beta) break;
        }
    }

    if (!move) {
        score = -negascout(state, depth + 1, -beta, -alpha, -color, use_tt);
        alpha = max(alpha, score);
        ++generated;
    }
    
    return alpha;

}