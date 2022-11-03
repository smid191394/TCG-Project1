/**
 * Framework for Threes! and its variants (C++ 11)
 * threes.cpp: Main file for Threes!
 *
 * Author: Theory of Computer Games
 *         Computer Games and Intelligence (CGI) Lab, NYCU, Taiwan
 *         https://cgilab.nctu.edu.tw/
 */

#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include "board.h"
#include "action.h"
#include "agent.h"
#include "episode.h"
#include "statistics.h"

int main(int argc, const char* argv[]) {
	std::cout << "Threes! Demo: ";
	std::copy(argv, argv + argc, std::ostream_iterator<const char*>(std::cout, " "));
	std::cout << std::endl << std::endl;

	size_t total = 1200, block = 0, limit = 0;
	std::string slide_args, place_args;
	std::string load_path, save_path;

	bool learned = true;
	float pattern[2][70000] = { {0}, {0} };
	int trail[10000][9], step;

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		auto match_arg = [&](std::string flag) -> bool {
			auto it = arg.find_first_not_of('-');
			return arg.find(flag, it) == it;
		};
		auto next_opt = [&]() -> std::string {
			auto it = arg.find('=') + 1;
			return it ? arg.substr(it) : argv[++i];
		};
		if (match_arg("total")) {
			total = std::stoull(next_opt());
		} else if (match_arg("block")) {
			block = std::stoull(next_opt());
		} else if (match_arg("limit")) {
			limit = std::stoull(next_opt());
		} else if (match_arg("slide") || match_arg("play")) {
			slide_args = next_opt();
		} else if (match_arg("place") || match_arg("env")) {
			place_args = next_opt();
		} else if (match_arg("load")) {
			load_path = next_opt();
		} else if (match_arg("save")) {
			save_path = next_opt();
		} else if (match_arg("learn")) {
			learned = false;
		}

	}

	statistics stats(total, block, limit);

	if (load_path.size()) {
		std::ifstream in(load_path, std::ios::in);
		in >> stats;
		in.close();
		if (stats.is_finished()) stats.summary();
	}

	if(learned){
		std::ifstream in;
		in.open("td0.txt");
		for(int i=0; i<2; i++){
			for(int j=0; j<70000; j++){
				in >> pattern[i][j];
			}
		}
		in.close();
	}

	random_slider slide(slide_args);
	random_placer place(place_args);

	while (!stats.is_finished()) {
//		std::cerr << "======== Game " << stats.step() << " ========" << std::endl;
		slide.open_episode("~:" + place.name());
		place.open_episode(slide.name() + ":~");
		stats.open_episode(slide.name() + ":" + place.name());
		episode& game = stats.back();

		step = 0;
		while (true) {			
			//slide tile
			agent& who = game.take_turns(slide, place);
			action move = who.take_action(game.state(), &pattern[0], &trail[0], step, learned);
//			std::cerr << game.state() << "#" << game.step() << " " << who.name() << ": " << move << std::endl;

			//place tile
			if (game.apply_action(move) != true) break;
			if (who.check_for_win(game.state())) break;
		}

		//backward method
		if(!learned){
			float now_val;
			for(int i = step - 1; i>=0; i--){				
				now_val = 
				(trail[i][8] + 
				(pattern[0][trail[i+1][0]]+
				pattern[0][trail[i+1][1]]+
				pattern[0][trail[i+1][2]]+
				pattern[0][trail[i+1][3]]+
				pattern[1][trail[i+1][4]]+
				pattern[1][trail[i+1][5]]+
				pattern[1][trail[i+1][6]]+
				pattern[1][trail[i+1][7]]) -
				(pattern[0][trail[i][0]]+
				pattern[0][trail[i][1]]+
				pattern[0][trail[i][2]]+
				pattern[0][trail[i][3]]+
				pattern[1][trail[i][4]]+
				pattern[1][trail[i][5]]+
				pattern[1][trail[i][6]]+
				pattern[1][trail[i][7]]))*0.05/8;

				pattern[0][trail[i][0]]+=now_val;
				pattern[0][trail[i][1]]+=now_val;
				pattern[0][trail[i][2]]+=now_val;
				pattern[0][trail[i][3]]+=now_val;
				pattern[1][trail[i][4]]+=now_val;
				pattern[1][trail[i][5]]+=now_val;
				pattern[1][trail[i][6]]+=now_val;
				pattern[1][trail[i][7]]+=now_val;
			}
		}	

		agent& win = game.last_turns(slide, place);
		stats.close_episode(win.name());
		slide.close_episode(win.name());
		place.close_episode(win.name());

	}

	if(!learned){
		std::ofstream out;
		out.open("td0.txt");
		for(int i=0; i<2; i++){
			for(int j=0; j<70000; j++){
				out << pattern[i][j] << endl;
			}
		}
		out.close();
	}

	if (save_path.size()) {
		std::ofstream out(save_path, std::ios::out | std::ios::trunc);
		out << stats;
		out.close();
	}

	return 0;
}
