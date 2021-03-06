﻿#include "Player.h"
using std::unordered_map;
using std::pair;
using std::to_string;
Player::Player()
{
	pokemon_bag.resize(6);
	pokemon_store.resize(50);
}

void Player::put_pokemon_in_bag(pokemon_base * pok)
{
	if (bag_count < 6)
		pokemon_bag[bag_count++] = pok;
	else
		put_pokemon_in_store(pok);
}
void Player::put_pokemon_in_store(pokemon_base * pok)
{
	if (store_count == pokemon_store.size())pokemon_store.resize(pokemon_store.size() + 50);
	pokemon_store[store_count++] = pok;
}
string Player::out_pokemon_info()
{
	string res;
	int flag = 1;
	for (int i = 0; i < bag_count; i++) {
		if (flag)flag = 0; else res.append("$$");
		res.append(pokemon_bag[i]->out_pokemon_info());
	}
	if (bag_count == 0)res.append("-1");
	res.append("###");
	flag = 1;
	for (int i = 0; i < store_count; i++) {
		if (flag)flag = 0; else res.append("$$");
		res.append(pokemon_store[i]->out_pokemon_info());
	}
	if (store_count == 0)res.append("-1");
	return res;
}

void Player::fresh_pokemon_pos(const vector<int>& bag, const vector<int>& store)
{
	unordered_map<int, pair<pokemon_base*, int> > pos;//unique_id ptr pos
	for (int i = 0; i < bag_count; i++) {
		pos[pokemon_bag[i]->get_unique_id()] = pair<pokemon_base*, int>{ pokemon_bag[i],i };
	}
	for (int i = 0; i < store_count; i++) {
		pos[pokemon_store[i]->get_unique_id()] = pair<pokemon_base*, int>{ pokemon_store[i],10+i };
	}
	int delt = 0;
	if (bag[0] != -1) {
		bag_count = bag.size();
		for (int i = 0; i < bag.size(); ++i) {
			auto pok = pos[bag[i]];
			if (pok.second != i) {
				pokemon_bag[i] = pok.first;
				pokemon_bag[i]->need_update[0] = true;
			}
		}
	}
	else {
		bag_count = 0;
	}
	if (store[0] != -1) {
		store_count = store.size();
		if (store_count >= pokemon_store.size())pokemon_store.resize(store_count);
		for (int i = 0; i < store.size(); ++i) {
			auto pok = pos[store[i]];
			if (pok.second != 10 + i) {
				pokemon_store[i] = pok.first;
				pokemon_store[i]->need_update[0] = true;
			}
		}
	}
	else {
		store_count = 0;
	}

}

unordered_map<int, string> Player::get_sql_update_info()
{
	unordered_map<int, string> res;
		for (int i = 0; i < bag_count; i++) {
			pokemon_base *pok = pokemon_bag[i];
			string tem("");
			get_pokemon_update_string(pok, tem,i);
			if (tem != "")res[pok->get_unique_id()] = tem;
		}
		for (int i = 0; i < store_count; i++) {
			pokemon_base *pok = pokemon_store[i];
			string tem("");
			get_pokemon_update_string(pok, tem, i+10);
			if (tem != "")res[pok->get_unique_id()] = tem;
		}
		return res;
}



void Player::get_pokemon_update_string(pokemon_base *pok,string & res,int i)
{
	int first = 1;
	for (int j = 0; j < pok->need_update.size(); ++j)
		if (pok->need_update[j]) {
			switch (j)
			{
			case 0:
				if (first)first = 0; else res.append(",");
				res.append(" bag_store=");
				res.append(to_string(i));
				res.append(" ");
				break;
			case 1: {
				if (first)first = 0; else res.append(",");
				auto atti = pok->get_attribute();
				auto level = pok->get_levels();
				res.append("hp="); res.append(to_string(atti[0]));
				res.append(",attack="); res.append(to_string(atti[1]));
				res.append(",defence="); res.append(to_string(atti[2]));
				res.append(",speed="); res.append(to_string(atti[3]));
				res.append(",level="); res.append(to_string(level[0]));
				res.append(",exp_now="); res.append(to_string(level[1]));
				res.append(",exp_levelup="); res.append(to_string(level[2]));
				res.append(" ");
				break; }

			}
			pok->need_update[j] = false;
		}
}
pokemon_base * Player::find_pok_by_unique(int id)
{
	for (int i = 0; i < bag_count; ++i) {
		if (pokemon_bag[i]->get_unique_id() == id)return pokemon_bag[i];
	}
	for (int i = 0; i < store_count; ++i) {
		if (pokemon_store[i]->get_unique_id() == id)return pokemon_store[i];
	}
	return nullptr;
}

void Player::server_handle()
{
	for (int i = 0; i < 40; i++) {
		pokemon_store[i]->level_up(rand() % 15);
		pokemon_store[i]->need_update[1] = 1;

	}
}

vector<int> Player::three_unique_id()
{
	int sum = store_count + bag_count;
	vector<int> result;
	if (sum <= 3) {
		for (int i = 0; i < bag_count; ++i)result.push_back(pokemon_bag[i]->get_unique_id());
		for (int i = 0; i < store_count; ++i)result.push_back(pokemon_store[i]->get_unique_id());
		if (sum == 2) {
			result.push_back(-1);
		}
		else if (sum == 1) {
			result.push_back(-1);
			result.push_back(-1);
		}
		else if (sum == 0) {
			result = { -1,-1,-1 };
		}
		
	}
	else {
		int first, second, third;
		if (store_count >= 2) {
			first = pokemon_store[rand() % store_count]->get_unique_id();
			second= pokemon_store[rand() % store_count]->get_unique_id();
			while (second == first)second = pokemon_store[rand() % store_count]->get_unique_id();
			third =pokemon_bag[rand() % bag_count]->get_unique_id();
		}
		else if (store_count == 1) {
			first = pokemon_store[0]->get_unique_id();
			second= pokemon_bag[rand() % bag_count]->get_unique_id();
			third = pokemon_bag[rand() % bag_count]->get_unique_id();
			while(second==third)third = pokemon_bag[rand() % bag_count]->get_unique_id();
		}
		else {
			first= pokemon_bag[rand() % bag_count]->get_unique_id();
			second= pokemon_bag[rand() % bag_count]->get_unique_id();
			while(second==first)second = pokemon_bag[rand() % bag_count]->get_unique_id();
			third =  pokemon_bag[rand() % bag_count]->get_unique_id();
			while(third==first||third==second)third = pokemon_bag[rand() % bag_count]->get_unique_id();
		}
		result = { first,second,third };
	}

	return result;
}

void Player::delet_pok(int unique)
{
	for (int i = 0; i < bag_count; ++i) {
		if (pokemon_bag[i]->get_unique_id() == unique) {
			
			for (int j = i; j < bag_count-1; j++) {
				pokemon_bag[j] = pokemon_bag[j + 1];
			}
			bag_count--;
			return;
		}
	}
	for (int i = 0; i < store_count; ++i) {
		if (pokemon_store[i]->get_unique_id() == unique) {
			for (int j = i; j < bag_count - 1; j++) {
				pokemon_store[j] = pokemon_store[j + 1];
			}
			store_count--;
			return;
		}
	}
}

int Player::fresh_madels()
{
	int res=0;
	int poknum = store_count + bag_count;
	int pokh=0;
	for (int i = 0; i < bag_count; i++) {
		if (pokemon_bag[i]->get_levels()[0] == 15)pokh++;
	}
	for (int i = 0; i < store_count; i++) {
		if (pokemon_store[i]->get_levels()[0] == 15)pokh++;
	}
	if (poknum >= 50)
		res += 70;
	else if (poknum >= 30) 
		res += 30;
	else if (poknum >= 10) 
		res += 10;
	if (pokh >= 12)
		res += 7;
	else if (pokh >= 8) 
		res += 3;
	
	else if (pokh >= 4) 
		res += 1;
	return res;
}






