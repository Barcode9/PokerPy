#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
#include <map>
#include <array>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <stdint.h>
#include <cmath>
#include "global.h"

using namespace std;
namespace py = pybind11;
using namespace pybind11::literals;

vector<map<string,int>> calculate_hand_frequency(vector<vector<Card>> cards) {

    int num_given_cards = cards[0].size();
    vector<array<Card,7>> players_cards;

    // Sort the cards and place them in arrays of 7 cards
    for (int i = 0; i < cards.size(); i++) {
        array<Card,7> temp;
        sort(cards[i].begin(), cards[i].end(), [](Card &a, Card &b){ return a.value > b.value; });
        copy(cards[i].begin(), cards[i].end(), temp.begin());
        players_cards.push_back(temp);
    }

    // Create the new hand array for passing it to the get_best_hand Function
    array<Card,7> new_hand;
    string possible_hand_types[10] = {"Royal Flush", "Straight Flush", "Poker", "Full House", "Flush", "Straight", "Triples", "Double Pairs", "Pairs", "High Card"};

    // Create the map with the hand_types and the number of hands of that type
    vector<map<string, int>> players_hand_posibilities;
    map<string, int> hand_posibilities;

    for (int i = 0; i < 10; i++) {
        hand_posibilities[possible_hand_types[i]] = 0;
    }
    hand_posibilities["Win"] = 0;
    hand_posibilities["Draw"] = 0;

    // Tie tracking between specific players
    map<string, int> tie_cases;

    // Initialize the players' hand possibilities
    for (int l = 0; l < players_cards.size(); l++) {
        players_hand_posibilities.push_back(hand_posibilities);
    }

    Hand result;
    // Create all possible cards
    vector<Card> possible_cards;
    for (int j = 13; j > 0; j--) {
        for (int i = 4; i > 0; i--) {
            Card new_card;
            bool already_in_hand = false;
            for (int l = 0; l < players_cards.size(); l++) {
                for (int k = 0; k < num_given_cards; k++) {
                    if (players_cards[l][k].value == j && players_cards[l][k].suit == i) {
                        already_in_hand = true;
                        break;
                    }
                }
            }
            if (!already_in_hand) {
                new_card.value = j;
                new_card.suit = (Suit)i;
                possible_cards.push_back(new_card);
            }
        }
    }

    array<int, 5> indexes = {0, 1, 2, 3, 4};
    int n = (7 - num_given_cards);
    int N = possible_cards.size();
    int num_possible_cases = 1;
    int intersected_cards = 0;
    int player_hand_heuristic = 0;
    array<int, 10> drawn_players_indx = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    while (true) {
        int max_hand_heuristic = 0;
        int drawn_players = 0;
        vector<int> current_tie_players;

        for (int l = 0; l < players_cards.size(); l++) {
            // Sort efficiently the hand cards for efficiency
            intersected_cards = 0;
            for (int i = 0; i < 7; i++) {
                if (intersected_cards < num_given_cards) {
                    if (i - intersected_cards >= n) {
                        new_hand[i] = players_cards[l][intersected_cards];
                        intersected_cards++;
                        continue;
                    } else if (players_cards[l][intersected_cards].value >= possible_cards[indexes[i - intersected_cards]].value) {
                        new_hand[i] = players_cards[l][intersected_cards];
                        intersected_cards++;
                        continue;
                    }
                }
                new_hand[i] = possible_cards[indexes[i - intersected_cards]];
            }

            result = get_best_hand(new_hand);
            players_hand_posibilities[l][hand_names[result.hand_type - 1]]++;
            // Check if win or draw
            player_hand_heuristic = result.hand_heuristic();
            if (player_hand_heuristic > max_hand_heuristic) {
                max_hand_heuristic = player_hand_heuristic;
                drawn_players = 1;
                drawn_players_indx[0] = l;
                current_tie_players.clear();
                current_tie_players.push_back(l);
            } else if (player_hand_heuristic == max_hand_heuristic) {
                drawn_players_indx[drawn_players] = l;
                drawn_players++;
                current_tie_players.push_back(l); // Add this player to the current tie scenario
            }
        }

        if (drawn_players == 1) {
            players_hand_posibilities[drawn_players_indx[0]]["Win"]++;
        } else {
            // Update each player involved in the draw
            for (int i = 0; i < drawn_players; i++) {
                players_hand_posibilities[drawn_players_indx[i]]["Draw"]++;
            }

            // Update the tie case scenario in the tie_cases map
            sort(current_tie_players.begin(), current_tie_players.end());
            string tie_case = "Player";
            for (int i = 0; i < current_tie_players.size(); ++i) {
                tie_case += " " + to_string(current_tie_players[i] + 1);
                if (i != current_tie_players.size() - 1) {
                    tie_case += ", ";
                }
            }
            tie_cases[tie_case]++;
        }

        num_possible_cases++;

        if (indexes[0] == N - n) {
            break;
        }

        // Create a new combination of indexes
        // Iterate backwards through the indexes
        for (int i = 1; i <= n; i++) {
            if (indexes[n - i] < N - i) {
                indexes[n - i]++;
                for (int j = n - i + 1; j < n; j++) {
                    indexes[j] = indexes[j - 1] + 1;
                }
                break;
            }
        }
    }

    // Add total cases for each player
    for (int l = 0; l < players_cards.size(); l++) {
        players_hand_posibilities[l]["Total Cases"] = num_possible_cases;
    }

    // Output the tie cases if needed
    for (const auto &tie_case : tie_cases) {
        cout << tie_case.first << " tie: " << tie_case.second << " times\n";
    }

    return players_hand_posibilities;
}

PYBIND11_MODULE(PokerPy, m) {
    m.doc() = "pybind11 plugin for calculating poker probabilities."; // optional module docstring

    py::class_<Card>(m, "Card")
        .def(py::init<short, short>())
        .def(py::init<string>())
        .def_property("value", [](Card& a){return card_names[a.value - 1];}, [](Card& a, string v){return a.value = card_values[v];})
        .def_property("suit", [](Card& a){return suit_names[a.suit - 1];}, [](Card& a, string s){return a.suit = suit_values[s];})
        .def("__repr__", &Card::to_string)
        .def("__eq__", &Card::operator==)
        .def("__ge__", &Card::operator>=);

    py::class_<Hand>(m, "Hand")
        .def(py::init<short, array<Card,5>>())
        .def(py::init<string, array<Card,5>>())
        .def_property("hand_type", [](Hand& a){return hand_names[a.hand_type - 1];}, [](Hand& a, string ht){return a.hand_type = hand_values[ht];})
        .def_readwrite("Cards", &Hand::Cards)
        .def("hand_heuristic", &Hand::hand_heuristic)
        .def("__repr__", &Hand::to_string)
        .def("__eq__", &Hand::operator==)
        .def("__ge__", &Hand::operator>=);

    m.def("get_best_hand", &get_best_hand_not_sorted, "A function that gets the best hands given 7 cards");
    m.def("calculate_hand_frequency", &calculate_hand_frequency, "A function that gets the frequencies of the possible hands given any number of cards");
}
