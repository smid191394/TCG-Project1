/**
 * Framework for Threes! and its variants (C++ 11)
 * agent.h: Define the behavior of variants of agents including players and environments
 *
 * Author: Theory of Computer Games
 *         Computer Games and Intelligence (CGI) Lab, NYCU, Taiwan
 *         https://cgilab.nctu.edu.tw/
 */

#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include <fstream>
#include "board.h"
#include "action.h"
#include "weight.h"
#include <iostream>

using namespace std;

class agent {
public:
	agent(const std::string& args = "") {
		std::stringstream ss("name=unknown role=unknown " + args);
		for (std::string pair; ss >> pair; ) {
			std::string key = pair.substr(0, pair.find('='));
			std::string value = pair.substr(pair.find('=') + 1);
			meta[key] = { value };
		}
	}
	virtual ~agent() {}
	virtual void open_episode(const std::string& flag = "") {}
	virtual void close_episode(const std::string& flag = "") {}
	virtual action take_action(const board& b, float (*pattern)[70000], int (*trail)[9], int& step, bool learned) { return action(); }
	virtual bool check_for_win(const board& b) { return false; }

public:
	virtual std::string property(const std::string& key) const { return meta.at(key); }
	virtual void notify(const std::string& msg) { meta[msg.substr(0, msg.find('='))] = { msg.substr(msg.find('=') + 1) }; }
	virtual std::string name() const { return property("name"); }
	virtual std::string role() const { return property("role"); }

protected:
	typedef std::string key;
	struct value {
		std::string value;
		operator std::string() const { return value; }
		template<typename numeric, typename = typename std::enable_if<std::is_arithmetic<numeric>::value, numeric>::type>
		operator numeric() const { return numeric(std::stod(value)); }
	};
	std::map<key, value> meta;
};

/**
 * base agent for agents with randomness
 */
class random_agent : public agent {
public:
	random_agent(const std::string& args = "") : agent(args) {
		if (meta.find("seed") != meta.end())
			engine.seed(int(meta["seed"]));
	}
	virtual ~random_agent() {}

protected:
	std::default_random_engine engine;
};

/**
 * base agent for agents with weight tables and a learning rate
 */
class weight_agent : public agent {
public:
	weight_agent(const std::string& args = "") : agent(args), alpha(0) {
		if (meta.find("init") != meta.end())
			init_weights(meta["init"]);
		if (meta.find("load") != meta.end())
			load_weights(meta["load"]);
		if (meta.find("alpha") != meta.end())
			alpha = float(meta["alpha"]);
	}
	virtual ~weight_agent() {
		if (meta.find("save") != meta.end())
			save_weights(meta["save"]);
	}

protected:
	virtual void init_weights(const std::string& info) {
		std::string res = info; // comma-separated sizes, e.g., "65536,65536"
		for (char& ch : res)
			if (!std::isdigit(ch)) ch = ' ';
		std::stringstream in(res);
		for (size_t size; in >> size; net.emplace_back(size));
	}
	virtual void load_weights(const std::string& path) {
		std::ifstream in(path, std::ios::in | std::ios::binary);
		if (!in.is_open()) std::exit(-1);
		uint32_t size;
		in.read(reinterpret_cast<char*>(&size), sizeof(size));
		net.resize(size);
		for (weight& w : net) in >> w;
		in.close();
	}
	virtual void save_weights(const std::string& path) {
		std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
		if (!out.is_open()) std::exit(-1);
		uint32_t size = net.size();
		out.write(reinterpret_cast<char*>(&size), sizeof(size));
		for (weight& w : net) out << w;
		out.close();
	}

protected:
	std::vector<weight> net;
	float alpha;
};

/**
 * default random environment, i.e., placer
 * place the hint tile and decide a new hint tile
 */
class random_placer : public random_agent {
public:
	random_placer(const std::string& args = "") : random_agent("name=place role=placer " + args) {
		spaces[0] = { 12, 13, 14, 15 };
		spaces[1] = { 0, 4, 8, 12 };
		spaces[2] = { 0, 1, 2, 3};
		spaces[3] = { 3, 7, 11, 15 };
		spaces[4] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
	}

	virtual action take_action(const board& after, float (*pattern)[70000], int (*trail)[9], int& step, bool learned) {
		std::vector<int> space = spaces[after.last()];
		std::shuffle(space.begin(), space.end(), engine);
		for (int pos : space) {
			if (after(pos) != 0) continue;

			int bag[3], num = 0;
			for (board::cell t = 1; t <= 3; t++)
				for (size_t i = 0; i < after.bag(t); i++)
					bag[num++] = t;
			std::shuffle(bag, bag + num, engine);

			board::cell tile = after.hint() ?: bag[--num];
			board::cell hint = bag[--num];

			return action::place(pos, tile, hint);
		}
		return action();
	}

private:
	std::vector<int> spaces[5];
};

/**
 * random player, i.e., slider
 * select a legal action randomly
 */
class random_slider : public random_agent {
public:
	random_slider(const std::string& args = "") : random_agent("name=slide role=slider " + args),
	opcode({ 0, 1, 2, 3 }) {}


	int translate(int a, int b, int c, int d){
		return a*4096+b*256+c*16+d;
	}
	virtual action take_action(const board& before, float (*pattern)[70000], int (*trail)[9], int& step, bool learned) {
		/*
		cout<<step<<endl;
			for(int i=0; i<4; i++){
				for(int j=0; j<4; j++){
					cout<<before[i][j]<<" ";
			}
			cout<<endl;
		}
		cout<<endl;*/
		int act = 0;
		float now_val, after_val;		
		board after[4] = {before, before, before, before};
		float reward[4];

		
		now_val = -1;
		for(int op:opcode){
			//compute after board
			reward[op]=(float)after[op].slide(op);
			
			//choose direction
			after_val = 
			reward[op] + 
			pattern[0][translate(after[op][0][0], after[op][0][1], after[op][0][2], after[op][0][3])] +
			pattern[0][translate(after[op][0][3], after[op][1][3], after[op][2][3], after[op][3][3])] +
			pattern[0][translate(after[op][3][3], after[op][3][2], after[op][3][1], after[op][3][0])] +
			pattern[0][translate(after[op][3][0], after[op][2][0], after[op][1][0], after[op][0][0])] +
			pattern[1][translate(after[op][1][0], after[op][1][1], after[op][1][2], after[op][1][3])] +
			pattern[1][translate(after[op][0][2], after[op][1][2], after[op][2][2], after[op][3][2])] +
			pattern[1][translate(after[op][2][3], after[op][2][2], after[op][2][1], after[op][2][0])] +
			pattern[1][translate(after[op][3][1], after[op][2][1], after[op][1][1], after[op][0][1])];
			if(after_val==0){
				act=op;
				break;
			}
			if(after_val>now_val){
				act = op;
				now_val = after_val;
			}
		}

		
		if(!learned){
			trail[step][0] = translate(after[act][0][0], after[act][0][1], after[act][0][2], after[act][0][3]);
			trail[step][1] = translate(after[act][0][3], after[act][1][3], after[act][2][3], after[act][3][3]);
			trail[step][2] = translate(after[act][3][3], after[act][3][2], after[act][3][1], after[act][3][0]);
			trail[step][3] = translate(after[act][3][0], after[act][2][0], after[act][1][0], after[act][0][0]);
			trail[step][4] = translate(after[act][1][0], after[act][1][1], after[act][1][2], after[act][1][3]);
			trail[step][5] = translate(after[act][0][2], after[act][1][2], after[act][2][2], after[act][3][2]);
			trail[step][6] = translate(after[act][2][3], after[act][2][2], after[act][2][1], after[act][2][0]);
			trail[step][7] = translate(after[act][3][1], after[act][2][1], after[act][1][1], after[act][0][1]);
			if(reward[act] == -1) trail[step][8] = 0;
			else trail[step][8] = reward[act];
		}

		if (reward[act] != -1){
			step++;			
			return action::slide(act);
		}
		return action();
	}

private:
	std::array<int, 4> opcode;
};


