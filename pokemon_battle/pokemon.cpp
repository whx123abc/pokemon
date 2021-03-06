﻿#include "pokemon.h"
#include <iostream>
using std::cout;
using std::endl;
using std::to_string;
string pokemon_r::use_skills(pokemon_base * enemy)
{
	auto &skills = this->get_skills();
	string skill_used;
	//skills[0]->skill_use(this,enemy);
	int flag = 1;
	for (auto &skill : skills) {
		if (skill) { 
			int used=skill->skill_use(this, enemy); 
			if (used) {
				if (flag)flag = 0; else skill_used.append(",");
				skill_used.append(to_string(skill->get_id()));
			}
		}
	}
	if (flag)skill_used.append(to_string(-1));
	skill_used.append("&&");
	return skill_used;
}





string pokemon_base::status_fresh()
{
	string res;
	int flag = 1;
	auto& status = this->get_status();
	vector<int> to_delete;
	for (auto &i : status) {
		auto &it = i.second;
		vector<int> status_str;
		--it[1];
		if (!it[1])to_delete.push_back(i.first);
		switch (i.first) {
		case 1: {
			if (!it[1]) {
				auto& atti_z = this->get_attribute_z();
				atti_z[1] -= it[2];
				atti_z[2] -= it[3];
			}
			break;
		}
		case 101: {
			if (!it[1]) {
				break;
			}
			attributes_z[0] -= it[2];
			status_str.push_back(i.first);
			status_str.push_back(it[2]);
			cout << "因为中毒受到了" << it[2] << "点伤害\n";
			break;
		}
		case 102: {
			if (!it[1]) {
				auto& atti_z = this->get_attribute_z();
				atti_z[2] += it[2];
			}
			break;
		}
		}
		int size = static_cast<int>(status_str.size()) ;
		if (size) {
			if (flag)flag = 0; else res.append("<>");
		}
		int dot_flag = 1;
		for (int i = 0; i < size; i++) {
			if (dot_flag)dot_flag = 0; else res.append(",");
			res.append(to_string(status_str[i]));
		}

	}
	if (flag)res.append(to_string(-1));
	res.append("&&");
	for (auto &it : to_delete)status.erase(it);
	return res;
}

bool pokemon_base::get_true_at_rate(double rate)
{ 
	return rate > (rand() % 10000)*1.0 / 10000;
}

bool pokemon_base::get_true_at_rate(int rate100)
{
	return get_true_at_rate(1.0*rate100 / 100);
}

void pokemon_base::battle_with(pokemon_base * enemy)
{
	repo.clear();
	
	int distence = 1000, source_distence = 0, enemy_distence = 0;
	this->atti_reset();
	enemy->atti_reset();
	vector<int> &source_atti_z = this->get_attribute_z();
	vector<int> &enemy_atti_z = enemy->get_attribute_z();
	int battle_continue = 1, win_flag;
	cout << "战斗开始了bro\n";
	this->out_z_status();
	enemy->out_z_status();
	while (battle_continue) {
		int is_battled = 0;
		source_distence += source_atti_z[3];
		enemy_distence += enemy_atti_z[3];
		if (source_distence >= enemy_distence && source_distence > distence) {
			is_battled = 1;
			cout << "\n我方行动中！" << endl;
			source_distence -= distence;
			repo.append("0&&");
			string turn_str= attack_turn(enemy);
			repo.append(turn_str);

			
		}
		if (enemy_distence >= source_distence && enemy_distence > distence) {
			cout << "\n敌方行动中！" << endl;
			is_battled = 1;
			enemy_distence -= distence;
			repo.append("1&&");
			string turn_str=enemy->attack_turn(this);
			repo.append(turn_str);

			
		}
		if (is_battled) {
			string atti_str;
			for (int i = 0; i < 6; i++) {
				if (i)atti_str.append(",");
				atti_str.append(to_string(source_atti_z[i]));
			}
			atti_str.append("<>");
			for (int i = 0; i < 6; i++) {
				if (i)atti_str.append(",");
				atti_str.append(to_string(enemy_atti_z[i]));
			}
			repo.append(atti_str);
			repo.append("###");
			this->out_z_status();
			enemy->out_z_status();
			if (enemy_atti_z[0] <= 0) {
				win_flag = 1;
				break;
			}else if (source_atti_z[0] <= 0) {
				win_flag = 0;
				break;
			}
		}
	}
	repo.append("FINAL");
	repo.insert(0, to_string(!win_flag) + string("###"));

	if (win_flag)cout << "老弟真的猛\n";
	else cout << "哈哈被打丢了吧\n";

	this->get_exp(win_flag ? 150 : 50);
	enemy->get_exp(win_flag ? 50 : 150);
	cout << repo<<repo.size();
}

string pokemon_base::attack_turn(pokemon_base * enemy)
{
	vector<int> &source_atti_z = get_attribute_z();
	vector<int> &enemy_atti_z = enemy->get_attribute_z();
	string skill_used;
	skill_used=use_skills(enemy);
	


	int demage = source_atti_z[1] - enemy_atti_z[2];
	demage = demage >= 0 ? demage : int(source_atti_z[1] * 0.1);

	string type_status;
	int type_change = 0;
	switch (source_atti_z[6] * 10 + enemy_atti_z[6])	//水火草普通
	{
	case 1:
	case 12:
	case 20:type_status = "效果极佳。"; type_change = demage / 4; break;
	case 10:
	case 21:
	case 02:type_status = "效果不好。"; type_change = -demage / 5; break;
	}
	int is_miss = 0, critical_increase = 0;
	if (get_true_at_rate(source_atti_z[4])) {
		critical_increase = demage / 2;
		cout << "造成了暴击！\n";
	}
	else if (get_true_at_rate(enemy_atti_z[5])) {
		is_miss = 1;
		cout << "对方闪避了伤害！\n";
	}
	if (is_miss)demage = 0;
	else {
		demage = demage + type_change + critical_increase;
		cout << "我方造成了" << demage << "点伤害!" << type_status << endl;
	}
	int tem_type;
	if (type_change > 0)tem_type = 1; else if (type_change == 0) tem_type = 0; else tem_type = -1;
	string dot = ",";
	string demage_str = to_string(critical_increase ? 1 : 0) + dot + to_string(is_miss)
		+ dot + to_string(tem_type) + dot + to_string(demage) + string("&&");


	enemy_atti_z[0] -= demage;
	string status_str;
	status_str= this->status_fresh();
	return skill_used + status_str + demage_str;
}

void pokemon_base::out_status()
{

	
	string type, _class;
	switch (attributes[6])
	{
	case 0:type = "水属性"; break;
	case 1:type = "火属性"; break;
	case 2:type = "草属性"; break;
	case 3:type = "普通属性"; break;
	}
	switch (attributes[7])
	{
	case 0:_class = "肉盾型"; break;
	case 1:_class = "力量型"; break;
	case 2:_class = "防御型"; break;
	case 3:_class = "敏捷型"; break;
	}
	cout << "Name:" << pokemon_name <<type<<" "<<_class <<endl;
	cout << "Level: " << levels[0] << " Exp_now: " << levels[1] << " Exp_levelup: " << levels[2] << endl;
	cout << "Attributes: HP:" << attributes[0] << " Attack: " << attributes[1] << " Defence: " << attributes[2]
		<< " Speed: " << attributes[3] << " Critical: " << attributes[4] << " Miss: " << attributes[5] << endl;
	cout << "Attributes: HP:" << attributes_z[0] << " Attack: " << attributes_z[1] << " Defence: " << attributes_z[2]
		<< " Speed: " << attributes_z[3] << " Critical: " << attributes_z[4] << " Miss: " << attributes_z[5] << endl;

}

void pokemon_base::out_z_status()
{
	cout << "Name:" << pokemon_name << endl;
	cout << "Attributes: HP:" << attributes_z[0] << " Attack: " << attributes_z[1] << " Defence: " << attributes_z[2]
		<< " Speed: " << attributes_z[3] << " Critical: " << attributes_z[4] << " Miss: " << attributes_z[5] << endl;
}

void pokemon_base::get_exp(int exp)
{
	cout << pokemon_name << "得到" << exp << "点经验";
	int tem = levels[0];
	while (levels[1] + exp >= levels[2]) {
		if (levels[0] < 15)++levels[0];
		exp= levels[1] + exp - levels[2];
		levels[1] = 0;
		levels[2] = levels[0] * 200;		//下一级升级经验
		level_up();
	}
	levels[1] += exp;
	if (levels[0] != tem)cout << "升级!";
	cout << endl;
	cout << "Level: " << levels[0] << " Exp_now: " << levels[1] << " Exp_levelup: " << levels[2] << endl;
}

void pokemon_base::level_up()
{
	int _class = attributes[7];
	for (int i = 0; i < 6; i++) {
		if (i == _class)attributes[i] += int(attributes[i] * 0.06);
		else attributes[i] += int(attributes[i] * 0.05);
	}
	
}

void pokemon_base::set_skill(int cur, int id, string name, string descrip, vector<int> skill_args)
{
	skill_base *skill_set = new skill_base(name,descrip , id, skill_args);
	this->get_skills()[cur] = skill_set;
}
