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

Hand get_best_hand(array<Card,7> cards){
    // WARNING This function assumes the cards are sorted before hand
    // Divide the cards by suit
    array<array<Card,7>,4> color_cards;
    array<int,4> num_color_cards = {0,0,0,0};
    color_cards[cards[0].suit - 1][0] = cards[0];
    num_color_cards[cards[0].suit - 1]++;
    // To get the straights
    array<Card,5> current_straight;
    current_straight[0] = cards[0];
    int current_straight_size = 1;
    array<Card,5> straight;
    bool found_straight = false;
    // To get straight flushes
    array<array<Card,5>,4> straight_flushes;
    straight_flushes[cards[0].suit - 1][0] = cards[0];
    array<int,4> straight_flushes_sizes = {0,0,0,0};
    straight_flushes_sizes[cards[0].suit - 1] += 1;
    array<Card,5> straight_flush;
    bool found_straight_flush = false;
    // To get rest of groupings
    array<array<Card,2>,3> pairs;
    int num_pairs = 0;
    array<array<Card,3>,2> triples;
    int num_triples = 0;
    array<Card,4> pokers;
    bool found_poker = false;
    array<Card,7> individuals;
    int num_individuals = 0;
    // Temporary variables;
    array<Card,4> group;
    int group_size = 1;
    group[0] = cards[0];
    Card last_card = cards[0];
    // Iterate through the sorted cards to extract the possible hands
    for(int i=1; i<7; i++)
    {
        // Search for pairs,triples,etc
        if (cards[i].value == last_card.value){
            group[group_size] = cards[i];
            group_size++;
        }else{
            switch (group_size)
            {
            case 1:
                individuals[num_individuals] = group[0];
                num_individuals++;
                break;
            case 2:
                pairs[num_pairs][0] = group[0];
                pairs[num_pairs][1] = group[1];
                num_pairs++;
                break;
            case 3:
                copy(group.begin(),group.begin()+3,triples[num_triples].begin());
                num_triples++;
                break;
            case 4:
                pokers = group;
                found_poker = true;
                break;
            }
            group_size = 1;
            group[0] = cards[i];
        }
        // Search for straights
        if (!found_straight){
            if (last_card.value - cards[i].value == 1){
                current_straight[current_straight_size] = cards[i];
                current_straight_size++;
            }else if (cards[i].value != last_card.value){
                current_straight_size = 1;
                current_straight[0] = cards[i];
            }
            if (current_straight_size == 5){
                straight = current_straight;
                current_straight_size = 0;
                found_straight = true;
            }
        }
        // Search for flushes
        color_cards[cards[i].suit - 1][num_color_cards[cards[i].suit - 1]] = cards[i];
        num_color_cards[cards[i].suit - 1]++;
        // Search for straight flushes
        if (straight_flushes_sizes[cards[i].suit - 1] != 0){
            if (straight_flushes[cards[i].suit - 1][straight_flushes_sizes[cards[i].suit - 1]-1].value - cards[i].value == 1){
                straight_flushes[cards[i].suit - 1][straight_flushes_sizes[cards[i].suit - 1]] = cards[i];
                straight_flushes_sizes[cards[i].suit - 1]++;
                if (straight_flushes_sizes[cards[i].suit - 1] == 5){
                    straight_flush = straight_flushes[cards[i].suit - 1];
                    found_straight_flush = true;
                    straight_flushes_sizes[cards[i].suit - 1] = 0;
                }
            }else{
                straight_flushes_sizes[cards[i].suit - 1] = 1;
                straight_flushes[cards[i].suit - 1][0] = cards[i];
            }
        }else{
            straight_flushes[cards[i].suit - 1][0] = cards[i];
            straight_flushes_sizes[cards[i].suit - 1]++;
        }
        last_card = cards[i];
    }
    // Add the last group
    switch (group_size)
    {
    case 1:
        individuals[num_individuals] = group[0];
        num_individuals++;
        break;
    case 2:
        pairs[num_pairs][0] = group[0];
        pairs[num_pairs][1] = group[1];
        num_pairs++;
        break;
    case 3:
        copy(group.begin(),group.begin()+3,triples[num_triples].begin());
        num_triples++;
        break;
    case 4:
        pokers = group;
        found_poker = true;
        break;
    }
    // Check for straight flushes
    if (!found_straight_flush){
        // Iterate through the cards to see if ACES of some suit makes a straight flush
        for(int i=0; i<7; i++)
        {
            if (cards[i].value != 13){
                break;
            }else if (straight_flushes_sizes[cards[i].suit - 1] == 4 && straight_flushes[cards[i].suit - 1][0].value == 4){
                copy(straight_flushes[cards[i].suit - 1].begin(),straight_flushes[cards[i].suit - 1].begin()+4,straight_flush.begin());
                straight_flush[4] = cards[i];
                found_straight_flush = true;
                break;
            }
        }
    }
    // Create the result hand
    Hand result;
    if (found_straight_flush){
        if (straight_flush[0].value == 13){
            result.hand_type = RoyalFlush;
        }else{
            result.hand_type = StraightFlush;
        }
        result.Cards = straight_flush;
        return result;
    }

    if (found_poker){
        result.hand_type = Poker;
        copy(pokers.begin(),pokers.end(),result.Cards.begin());
        for(int i=0; i<7; i++){
            if (cards[i].value != pokers[0].value){
                result.Cards[4] = cards[i];
                break;
            }
        }
        return result;
    }
    
    if (num_triples > 0 && num_pairs > 0){
        result.hand_type = FullHouse;
        copy(triples[0].begin(),triples[0].end(),result.Cards.begin());
        result.Cards[3] = pairs[0][0];
        result.Cards[4] = pairs[0][1];
        return result;
    }else if(num_triples == 2){
        result.hand_type = FullHouse;
        copy(triples[0].begin(),triples[0].end(),result.Cards.begin());
        result.Cards[3] = triples[1][0];
        result.Cards[4] = triples[1][1];
        return result;
    }
    // Check flushes
    for (int i = 0; i < 4; i++)
    {
        if (num_color_cards[i] >= 5){
            result.hand_type = Flush;
            copy(color_cards[i].begin(),color_cards[i].begin()+5,result.Cards.begin());
            return result;
        }
    }
    
    // Check if the straight form with an ACE
    if (!found_straight && current_straight_size == 4 && current_straight[3].value == 1 && cards[0].value == 13){
        copy(current_straight.begin(),current_straight.begin()+4,straight.begin());
        found_straight = true;
        straight[4] = cards[0];
    }
    
    if (found_straight){
        result.hand_type = Straight;
        result.Cards = straight;
        return result;
    }

    if (num_triples == 1){
        result.hand_type = Triples;
        copy(triples[0].begin(),triples[0].begin()+3,result.Cards.begin());
        result.Cards[3] = individuals[0];
        result.Cards[4] = individuals[1];
        return result;
    }

    if (num_pairs >= 2){
        result.hand_type = DoublePairs;
        result.Cards[0] = pairs[0][0];
        result.Cards[1] = pairs[0][1];
        result.Cards[2] = pairs[1][0];
        result.Cards[3] = pairs[1][1];
        result.Cards[4] = individuals[0];
        return result;
    }

    if (num_pairs == 1){
        result.hand_type = Pairs;
        result.Cards[0] = pairs[0][0];
        result.Cards[1] = pairs[0][1];
        result.Cards[2] = individuals[0];
        result.Cards[3] = individuals[1];
        result.Cards[4] = individuals[2];
        return result;
    }
    result.hand_type = HighCard;
    copy(individuals.begin(),individuals.begin()+5,result.Cards.begin());
    return result;
}

vector<map<string, int>> calculate_hand_frequency(vector<vector<Card>> cards, vector<vector<Card>> cards_to_remove) {
    int num_given_cards = cards[0].size();
    vector<array<Card, 7>> players_cards;

    // Sort the cards and place them in arrays of 7 cards
    for (int i = 0; i < cards.size(); i++) {
        array<Card, 7> temp;
        sort(cards[i].begin(), cards[i].end(), [](Card &a, Card &b) { return a.value > b.value; });
        copy(cards[i].begin(), cards[i].end(), temp.begin());
        players_cards.push_back(temp);
    }

    // Create the new hand array for passing it to the get_best_hand function
    array<Card, 7> new_hand;
    string possible_hand_types[10] = {"Royal Flush", "Straight Flush", "Poker", "Full House", "Flush", "Straight", "Triples", "Double Pairs", "Pairs", "High Card"};

    // Create the map with the hand_types and the number of hands of that type
    vector<map<string, int>> players_hand_posibilities;
    map<string, int> hand_posibilities;
    for (int i = 0; i < 10; i++) {
        hand_posibilities[possible_hand_types[i]] = 0;
    }
    hand_posibilities["Win"] = 0;
    hand_posibilities["Draw"] = 0;

    // Add tie cases tracking
    map<string, int> tie_cases;

    for (int l = 0; l < players_cards.size(); l++) {
        players_hand_posibilities.push_back(hand_posibilities);
    }

    Hand result;
    vector<Card> possible_cards;

    // Create all possible cards, excluding those already in hand and those in the cards_to_remove vector
    for (int j = 13; j > 0; j--) {
        for (int i = 4; i > 0; i--) {
            Card new_card;
            new_card.value = j;
            new_card.suit = (Suit)i;

            bool already_in_hand = false;
            bool to_be_removed = false;

            // Check if the card is already in a player's hand
            for (int l = 0; l < players_cards.size(); l++) {
                for (int k = 0; k < num_given_cards; k++) {
                    if (players_cards[l][k].value == j && players_cards[l][k].suit == i) {
                        already_in_hand = true;
                        break;
                    }
                }
                if (already_in_hand) break;
            }

            // Check if the card is in the cards_to_remove vector
            for (const auto &remove_set : cards_to_remove) {
                for (const auto &card_to_remove : remove_set) {
                    if (new_card.value == card_to_remove.value && new_card.suit == card_to_remove.suit) {
                        to_be_removed = true;
                        break;
                    }
                }
                if (to_be_removed) break;
            }

            // Only add the card if it is neither in hand nor in the removal list
            if (!already_in_hand && !to_be_removed) {
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
    array<int, 10> drawed_players_indx = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    while (true) {
        int max_hand_heuristic = 0;
        int drawed_players = 0;
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
                drawed_players = 1;
                drawed_players_indx[0] = l;
                current_tie_players.clear();
                current_tie_players.push_back(l);
            } else if (player_hand_heuristic == max_hand_heuristic) {
                drawed_players_indx[drawed_players] = l;
                drawed_players++;
                current_tie_players.push_back(l);  // Track the current tie scenario
            }
        }

        if (drawed_players == 1) {
            players_hand_posibilities[drawed_players_indx[0]]["Win"]++;
        } else {
            for (int i = 0; i < drawed_players; i++) {
                players_hand_posibilities[drawed_players_indx[i]]["Draw"]++;
            }

            // Sort the players involved in the tie
            sort(current_tie_players.begin(), current_tie_players.end());

            // Create a string to represent the tie case (e.g., "Ties with Player 2, Player 3")
            string tie_case = "Ties with";
            for (int i = 0; i < current_tie_players.size(); ++i) {
                tie_case += " Player " + to_string(current_tie_players[i] + 1);
                if (i != current_tie_players.size() - 1) {
                    tie_case += ",";
                }
            }

            // Track the tie case for each player involved in the tie
            for (int i = 0; i < current_tie_players.size(); i++) {
                players_hand_posibilities[current_tie_players[i]][tie_case]++;
            }
        }

        num_possible_cases++;

        if (indexes[0] == N - n) {
            break;
        }

        // Create a new combination of indexes
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

    for (int l = 0; l < players_cards.size(); l++) {
        players_hand_posibilities[l]["Total Cases"] = num_possible_cases;
    }

    return players_hand_posibilities;
}

Hand get_best_hand_not_sorted(array<Card,7> cards){
    sort(cards.begin(),cards.end(),[](Card &a,Card &b){return a.value > b.value;});
    return get_best_hand(cards);
}

string round_float(float a,int num_decimals){
    int temp = round(a*pow(10,num_decimals));
    string total_number = to_string((float) temp/(pow(10,num_decimals)));
    return total_number.substr(0,total_number.find(".")+2);
}

void nice_print_frequencies(vector<map<string,int>> frecs){
    // Print win/draw probabilities
    for (int i = 0; i < frecs.size(); i++){
        float win_chance = ((float) frecs[i]["Win"]/(float) frecs[i]["Total Cases"])*100;
        float draw_chance = ((float) frecs[i]["Draw"]/(float) frecs[i]["Total Cases"])*100;
        py::print("Player: ",i,", Win: ",round_float(win_chance,2),"%, Draw: ",round_float(draw_chance,2),"%","sep"_a="");
    }
    py::print("\nHAND POSSIBILITIES\n");
    // Print Table Headers 
    py::print("\t\t","end"_a="");
    for (int i = 0; i < frecs.size(); i++){
        py::print("Player",i,"\t","end"_a="","sep"_a="");
    }
    py::print("");
    // Print Each hand posibilities
    for (int i = 0; i < 10; i++){
        py::print(hand_names[i],"end"_a="");
        if (hand_names[i].size() > 7){
            py::print("\t","end"_a="");
        }else{
            py::print("\t\t","end"_a="");
        }
        for (int j = 0; j < frecs.size(); j++){
            float hand_pos = ((float) frecs[j][hand_names[i]]/(float) frecs[j]["Total Cases"])*100;;
            py::print(round_float(hand_pos,2),"%\t","end"_a="","sep"_a="");
        }
        py::print("");
    }
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
        .def(py::init<short, array<Card, 5>>())
        .def(py::init<string, array<Card, 5>>())
        .def_property("hand_type", [](Hand& a){return hand_names[a.hand_type - 1];}, [](Hand& a, string ht){return a.hand_type = hand_values[ht];})
        .def_readwrite("Cards", &Hand::Cards)
        .def("hand_heuristic", &Hand::hand_heuristic)
        .def("__repr__", &Hand::to_string)
        .def("__eq__", &Hand::operator==)
        .def("__ge__", &Hand::operator>=);

    m.def("get_best_hand", &get_best_hand_not_sorted, "A function that gets the best hands given 7 cards");

    // Updated function registration for calculate_hand_frequency
    m.def("calculate_hand_frequency", 
          &calculate_hand_frequency, 
          "A function that gets the frequencies of the possible hands given any number of cards and cards to remove",
          py::arg("cards"), py::arg("cards_to_remove"));

    m.def("nice_print_frequencies", &nice_print_frequencies, "A function that gets the frequencies of the possible hands and prints them in nice format");
}
