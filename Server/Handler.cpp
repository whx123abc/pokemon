﻿#include "Handler.h"
#include <string>
using std::string;
#define AD_NAME "AFMowqmt1ga21"
Handler::Handler(QObject *parent)
	: QObject(parent)
{
	srand(unsigned int(time(NULL)));
	driver = sql::mysql::get_mysql_driver_instance();
	if (driver == NULL) {
		emit string_to_socket_ready(QString("Driver failed"), 2);
	}
	con = driver->connect("tcp://localhost:3306", "root", "123456");
	if (con == NULL) {
		emit string_to_socket_ready(QString("Connect to sql failed"), 2);
	}
	stmt = con->createStatement();
	//stmt->execute("set character set gbk");
	stmt->execute("use pokemon_database");
	emit string_to_socket_ready(QString("SQL connect success"), 2);
}

Handler::~Handler()
{
}

void Handler::put_three_pokemons_in_bag()
{
	player.set_next_unique(0);
	res = stmt->executeQuery(QString("select count(*) num from pokemon_base where rarity=1;\0").toUtf8().data());
	QString info="information****注册赠送三只精灵：";
	for (int i = 0; i < 3; ++i) {	
		give_player_random_r(info);
		info.append(i == 2 ? "。" : "，");
	}
	emit string_to_socket_ready(info, 1);
}

void Handler::get_player_pokemons()
{
	res = stmt->executeQuery(QString("select * from pokemon_user where user_name='%1' order by bag_store\0").arg(user_name).toUtf8().data());
	int pokemon_num = 0;
	while (res->next()) {
		pokemon_base *tem = res_to_pokemon(res);
		int bag_store = 0;
		if (res->getInt("bag_store")<6) {
			player.put_pokemon_in_bag(tem);
		}
		else player.put_pokemon_in_store(tem);
	}

	res = stmt->executeQuery(QString("select * from users where user_name='%1'\0").arg(user_name).toUtf8().data());
	if (res->next())player.set_next_unique(res->getInt("next_unique"));
}

pokemon_base * Handler::res_to_pokemon(sql::ResultSet * res)
{
	pokemon_base *pok;
	string pokemon_name = res->getString("pokemon_name").c_str();
	vector<int> id{ res->getInt("id_unique") ,res->getInt("id_pokemon") };
	vector<int> atti{ res->getInt("hp"),res->getInt("attack"), res->getInt("defence"), res->getInt("speed"),
	res->getInt("critical"), res->getInt("miss"), res->getInt("type"), res->getInt("class") };
	vector<int> level{ res->getInt("level"), res->getInt("exp_now"), res->getInt("exp_levelup") };
	switch (res->getInt("rarity"))
	{
	case 1:
		pok = new pokemon_r(pokemon_name, id, atti, level);
		break;
	}
	vector<int> skills = { res->getInt("skill1"),res->getInt("skill2") };
	for (int i = 0; i < 2; ++i) {
		if (skills[i] != -1) {
			res2 = stmt->executeQuery(QString("select * from skill where id=%1;\0").arg(skills[i]).toUtf8().data());
			if (res2->next()) {
				pok->set_skill(i, res2->getInt("id"), "", "", vector<int>{res2->getInt("arg1"), res2->getInt("arg2"), res2->getInt("arg3"), res2->getInt("arg4")});
			}
		}
	}

	return pok;
}

pokemon_base* Handler::give_player_random_r(QString &info,int choic)
{
	pokemon_base *pok;
	res= stmt->executeQuery(QString("select count(*) num from pokemon_base where rarity=1\0").toUtf8().data());
	res->next();
	int num = res->getInt("num");
	int choice;
	if(choic==-1)
	 choice= rand() % num;
	else choice = choic;
	res = stmt->executeQuery(QString("select * from pokemon_base limit 1 offset %1\0").arg(choice).toUtf8().data());
	if (res->next()) {
		int skill_init = res->getInt("skill_init");
		QString pokemon_info_sql = QString("('%1',%2,%3,'%4',%5,%6,%7,%8,%9,%10,%11,%12,%13,1,0,200,%14,%15,-1,-1,-1)")
			.arg(user_name).arg(player.get_next_unique())
			.arg(res->getInt("id")).arg(res->getString("name").c_str()).arg(res->getInt("rarity"))
			.arg(res->getInt("class")).arg(res->getInt("type"))
			.arg(res->getInt("hp")).arg(res->getInt("attack")).arg(res->getInt("defence"))
			.arg(res->getInt("speed")).arg(res->getInt("critical")).arg(res->getInt("miss"))
			.arg(player.get_store_num()+10).arg(skill_init==-1?-1:skill_init);

		string name = res->getString("name").c_str();
		if(info!="")info.append(res->getString("name").c_str());
		vector<int> atti{ res->getInt("hp") ,res->getInt("attack"),res->getInt("defence"),
			res->getInt("speed"),res->getInt("critical"),res->getInt("miss"),res->getInt("type"),res->getInt("class") };
		vector<int> levels{ 1,0,200 };
		vector<int> id{ player.get_next_unique(),res->getInt("id") };
		player.unique_id_inc();
		pok = new pokemon_r(name, id, atti, levels);
		
		if (skill_init != -1) {
			res2 = stmt->executeQuery(QString("select * from skill where id=%1;\0").arg(skill_init).toUtf8().data());
			if (res2->next()) {
				pok->set_skill(0, res2->getInt("id"), "", "", vector<int>{res2->getInt("arg1"), res2->getInt("arg2"), res2->getInt("arg3"), res2->getInt("arg4")});
			}		
		}
		player.put_pokemon_in_store(pok);
		//emit string_to_socket_ready(QString("insert into pokemon_user values ") + pokemon_info_sql, 2);
		stmt->executeUpdate(QString("update users set next_unique=%1 where user_name='%2'").arg(player.get_next_unique()).arg(user_name).toUtf8().data());
		stmt->executeUpdate((QString("insert into pokemon_user values ") + pokemon_info_sql).toUtf8().data());
	}
	return pok;
}

void Handler::update_pokemon_sql()
{
	unordered_map<int, string > map = player.get_sql_update_info();
	for (auto &it : map) {
		QString str = QString("update pokemon_user set %1 where user_name='%2' and id_unique=%3").arg(QString::fromStdString(it.second)).arg(user_name).arg(it.first);
		stmt->executeUpdate(str.toUtf8().data());
	}
}

void Handler::server_handler()
{
	user_name = AD_NAME;
	player.set_user_name(AD_NAME);
	QString a;
	stmt->execute("delete from pokemon_user where user_name ='AFMowqmt1ga21'");
	//stmt->execute("update users set next_unique = 0 where user_name='AFMowqmt1ga21'");
	player.set_next_unique(0);
	for (int i = 0; i < 40; ++i) {
		give_player_random_r(a,i%8);
	}
	get_player_pokemons();
	player.server_handle();
	for (int i = 0; i < 20; i++)check_skill_update(player.find_pok_by_unique(i));
	update_pokemon_sql();
}

void Handler::send_battle_enemy()
{
	QString str_res="battle_pokemon****";
	res = stmt->executeQuery(QString("select * from pokemon_user where user_name='%1'").arg(AD_NAME).toUtf8().data());
	int flag = 1;
	while (res->next()) {
		if (flag)flag = 0; else str_res.append("###");
		str_res.append(res->getString("pokemon_name").c_str());
		str_res.append(" lv.");
		str_res.append(res->getString("level").c_str());
		QString type;
		switch (res->getInt("type"))
		{
		case 0:type = " 水属性"; break;
		case 1:type = " 火属性"; break;
		case 2:type = " 草属性"; break;
		case 3:type = " 普通属性"; break;
		}
		str_res.append(type);
	}
	emit string_to_socket_ready(str_res, 1);
}

void Handler::check_skill_update(pokemon_base *pok)
{
	for(int i=2;i<=4;++i)
	if (pok->need_update[i] == 1) {
		QString query,rand_skill;
		if (pok->get_attribute()[6] == 3) {
			query = "select count(*) num from skill where type=3 and init=0";
			rand_skill = "select * from skill where type=3 and init=0 limit 1 offset %1";
		}
		else {
			query = QString("select count(*) num from skill where (type=3 or type=%1) and init=0").arg(pok->get_attribute()[6]);
			rand_skill = QString("select * from skill where (type=3 or type=%1) and init=0 limit 1 offset %2").arg(pok->get_attribute()[6]);
		}
		res2 = stmt->executeQuery(query.toUtf8().data());
		res2->next();
		int skill_num = res2->getInt("num");
		res2 = stmt->executeQuery(rand_skill.arg(rand() % skill_num).toUtf8().data());
		if (res2->next()) {
			pok->set_skill(i-1, res2->getInt("id"), "", "", vector<int>{res2->getInt("arg1"), res2->getInt("arg2"), res2->getInt("arg3"), res2->getInt("arg4")});
		}
		stmt->execute(QString("update pokemon_user set skill%1=%2 where user_name='%3' and id_unique=%4\0")
			.arg(i).arg(res2->getInt("id")).arg(user_name).arg(pok->get_unique_id()).toUtf8().data());
		pok->need_update[i] = 0;
	}
	
}

void Handler::insert_pok_to_sql(pokemon_base * pok)
{
	auto &atti = pok->get_attribute();
	auto &level = pok->get_levels();
	auto &skill = pok->get_skills();
	int skill_num = skill.size();
	int skill3, skill4;
	if (skill_num >= 3) {
		skill3=skill[2] ? skill[2]->get_id() : -1;
	}
	else skill3 = -1;
	if (skill_num >= 4) {
		skill4 = skill[3] ? skill[3]->get_id() : -1;
	}
	else skill4 = -1;
	QString s = QString("insert into pokemon_user values ('%1',%2,%3,'%4',%5,%6,%7,%8,%9,%10,%11,%12,%13,%14,%15,%16,%17,%18,%19,%20,%21)")
		.arg(user_name).arg(player.get_next_unique()).arg(pok->get_pokemon_id()).arg(QString::fromStdString( pok->get_name()))
		.arg(pok->get_rarity()).arg(atti[7]).arg(atti[6]).arg(atti[0]).arg(atti[1]).arg(atti[2])
		.arg(atti[3]).arg(atti[4]).arg(atti[5]).arg(level[0]).arg(level[1]).arg(level[2])
		.arg(player.get_store_num()+10).arg(skill[0]?skill[0]->get_id():-1)
		.arg(skill[1] ? skill[1]->get_id() : -1).arg(skill3)
		.arg(skill4)
		;
	stmt->execute(s.toUtf8().data());
	player.unique_id_inc();
	stmt->executeUpdate(QString("update users set next_unique=%1 where user_name='%2'").arg(player.get_next_unique()).arg(user_name).toUtf8().data());
}

QString Handler::get_player_sql_info(const QString &user_name_)
{
	
	res = stmt->executeQuery(QString("select * from users where user_name='%1'\0").arg(user_name_).toUtf8().data());
	res->next();
	battle_num = res->getInt("battle_num");
	battle_win = res->getInt("battle_win");
	madels_poknum = res->getInt("madels_poknum");
	madels_pokh = res->getInt("madels_pokh");
	QString str_res = QString("player_info****%1,%2,%3,%4").arg(battle_num)
		.arg(battle_win).arg(madels_poknum).arg(madels_pokh);
	return str_res;
}

void Handler::update_sql_battle_num(int win_flag)
{
	battle_num++;
	battle_win += win_flag;
	stmt->executeUpdate(QString("update users set battle_num=%1,battle_win=%2 where user_name='%3'\0").arg(battle_num).arg(battle_win).arg(user_name).toUtf8().data());
}

void Handler::fresh_player_madels()
{
	int medal_status=player.fresh_madels();
	int tem_madels_poknum = medal_status / 10;
	int tem_madels_pokh = medal_status % 10;
	if (tem_madels_poknum > madels_poknum) {
		stmt->executeUpdate(QString("update users set madels_poknum=%1 where user_name='%2'\0").arg(tem_madels_poknum).arg(user_name).toUtf8().data());
		madels_poknum = tem_madels_poknum;
	}
	if (tem_madels_pokh > madels_pokh) {
		stmt->executeUpdate(QString("update users set madels_pokh=%1 where user_name='%2'\0").arg(tem_madels_pokh).arg(user_name).toUtf8().data());
		madels_pokh = tem_madels_pokh;
	}
}

void Handler::user_disconnect()
{
	stmt->executeUpdate(QString("update users set online=0 where user_name='%1' ").arg(user_name).toUtf8().data());
}

void Handler::get_string_from_socket(const QString & str)
{
	QStringList list = str.split("****");
	QString mode = list.at(0);
	if (mode == "login") {
		if (list.at(1).length() < 6 || list.at(2).length() < 6) {	//冗余设置
			emit string_to_socket_ready(QString("login****fail****用户名或密码长度不足6位，请重新输入。"), 1);
			return;
		}
		res = stmt->executeQuery(QString("select * from users where user_name='%1'\0").arg(list.at(1)).toUtf8().data());
		if (res->next()) {
			QString password = res->getString("password").c_str();
			if (password == list.at(2)) {
				user_name = list.at(1);
				player.set_user_name(user_name.toStdString());
				stmt->executeUpdate(QString("update users set online=1 where user_name='%1' ").arg(list.at(1)).toUtf8().data());
				emit string_to_socket_ready(QString("user %1 login success").arg(list.at(1)), 2);
				emit string_to_socket_ready(QString("login****success****登录成功"), 1);
				get_player_pokemons();
				fresh_player_madels();
				QString player_info = get_player_sql_info(user_name);
				emit string_to_socket_ready(player_info, 1);//player_info
				QString pokemon_str = QString::fromStdString(player.out_pokemon_info());
				emit string_to_socket_ready(QString("query_pokemon****") + pokemon_str, 1);
				send_battle_enemy();
			}
			else {
				emit string_to_socket_ready("login failed wrong password ", 2);
				emit string_to_socket_ready(QString("login****fail****密码错误，请检查后重新登录。"), 1);
			}
		}
		else {
			emit string_to_socket_ready("login failed", 2);
			emit string_to_socket_ready(QString("login****fail****用户名%1未被注册，请先注册用户。").arg(list.at(1)), 1);
		}

	}
	else if (mode == "register") {
		if (list.at(1).length() < 6 || list.at(2).length() < 6) {	//冗余设置
			emit string_to_socket_ready(QString("register****fail****用户名或密码长度不足6位，请重新输入。"), 1);
			return;
		}
		res = stmt->executeQuery(QString("select * from users where user_name='%1'\0").arg(list.at(1)).toUtf8().data());
		if (res->next()) {
			emit string_to_socket_ready("register failed", 2);
			emit string_to_socket_ready(QString("register****fail****用户名%1已被注册，请换用户名重新注册。").arg(list.at(1)), 1);
		}
		else {
			stmt->executeUpdate(QString("insert into users values ('%1','%2',1,0,0,3,0,0)").arg(list.at(1)).arg(list.at(2)).toUtf8().data());
			user_name = list.at(1);
			player.set_user_name(user_name.toStdString());

			emit string_to_socket_ready(QString("user %1 register success").arg(list.at(1)), 2);
			emit string_to_socket_ready(QString("register****success****恭喜您注册成功"), 1);
			put_three_pokemons_in_bag();
			QString pokemon_str = QString::fromStdString(player.out_pokemon_info());
			emit string_to_socket_ready(QString("query_pokemon****") + pokemon_str, 1);
			emit string_to_socket_ready(QString("player_info****0,0,0,0"), 1);//player_info
			send_battle_enemy();
		}
	}
	else if (mode == "query_player") {
		res = stmt->executeQuery("select * from users where online=1\0");
		QString res_str = "query_player****";
		int count = 0;
		while (res->next()) {
			if (count)res_str.append("###");
			++count;
			res_str.append(res->getString("user_name").c_str());
		}
		if (count == 0)res_str.append("目前无用户在线");
		emit string_to_socket_ready(QString("user %1 query player success").arg(user_name), 2);
		emit string_to_socket_ready(res_str, 1);
	}
	else if (mode == "query_pokemon") {//移到了登陆成功
		QString pokemon_str = QString::fromStdString(player.out_pokemon_info());
		emit string_to_socket_ready(QString("query_pokemon****") + pokemon_str, 1);
		emit string_to_socket_ready(QString("user %1 query pokemon success").arg(user_name), 2);
	}
	else if (mode == "query_player_pokemon") {
		res = stmt->executeQuery(QString("select * from pokemon_user where user_name = '%1'\0").arg(list.at(1)).toUtf8().data());
		QString query_res = "query_player_pokemon****";
		int flag = 1;
		while (res->next()) {
			if (flag)flag = 0; else query_res.append("###");
			query_res.append(res->getString("pokemon_name").c_str());
			query_res.append(QString("lv.%1,%2").arg(res->getInt("level")).arg(res->getInt("id_unique")));
		}
		if (flag)query_res.append("-1");
		QString win_madel = get_player_sql_info(list.at(1));
		query_res.append("****" + win_madel);
		emit string_to_socket_ready(query_res, 1);
		emit string_to_socket_ready(QString("%1 query %2 success").arg(user_name).arg(list.at(1)), 2);
	}
	else if (mode == "query_all_pokemon") {
		QString str_res="query_all_pokemon****";
		res = stmt->executeQuery(QString("select user_name,pokemon_name,level from pokemon_user where user_name!='AFMowqmt1ga21' \0").toUtf8().data());
		QString last_user_name="-1";
		while (res->next())
		{
			QString tem_name = QString::fromStdString(res->getString("user_name").c_str());
			if (last_user_name == "-1") {
				str_res.append(tem_name+"$$");
				last_user_name = tem_name;
			}
			else if (last_user_name != tem_name) {
				last_user_name = tem_name;
				str_res.append("-1###" + tem_name+"$$");
			}
			QString pok_name= QString::fromStdString(res->getString("pokemon_name").c_str());
			int level = res->getInt("level");
			str_res.append(QString("%1 lv.%2$$").arg(pok_name).arg(level));
		}
		str_res.append("-1");
		emit string_to_socket_ready(str_res, 1);
	}
	else if (mode == "gacha") {
		QStringList gacha_list = list.at(1).split("###");
		switch (gacha_list.at(0).toInt())
		{
		case 1:
			if (gacha_list.at(1).toInt() == 1) {
				QString str = "information****恭喜你获得";
				pokemon_base *pok = give_player_random_r(str);
				str.append("！");
				QString pokemon_str = QString("add_pokemon****") + QString::fromStdString(pok->out_pokemon_info());
				emit string_to_socket_ready(pokemon_str, 1);
				emit string_to_socket_ready(str, 1);
			}
		}
	}
	else if (mode == "pokemon_fresh") {

		QStringList temlist = list.at(1).split("###");
		QStringList baglist = temlist.at(0).split(","), storelist = temlist.at(1).split(",");
		vector<int> bag, store;
		for (int i = 0; i < baglist.size(); ++i)bag.push_back(baglist.at(i).toInt());
		for (int i = 0; i < storelist.size(); ++i)store.push_back(storelist.at(i).toInt());
		player.fresh_pokemon_pos(bag, store);
		update_pokemon_sql();
	}
	else if (mode == "battle_levelup") {
		QString str = QString("battle****");
		QStringList unique_ids = list.at(1).split("###");
		res = stmt->executeQuery(QString("select * from pokemon_user where user_name='%1' and id_unique =%2 \0").arg(AD_NAME).arg(unique_ids.at(1)).toUtf8().data());
		if (res->next()) {
			pokemon_base *enemy = res_to_pokemon(res);
			pokemon_base *source = player.find_pok_by_unique(unique_ids.at(0).toInt());
			str += QString::fromStdString(source->battle_with(enemy));
			int win_flag = str.split("****").at(1).split("###").at(0).toInt();
			update_sql_battle_num(win_flag);
			str += "****-1";
			check_skill_update(source);
			update_pokemon_sql();
			emit string_to_socket_ready(str, 1);

		}
	}
	else if (mode == "battle_duel") {
		QString str = QString("battle****");
		QStringList unique_ids = list.at(1).split("###");
		res = stmt->executeQuery(QString("select * from pokemon_user where user_name='%1' and id_unique =%2 \0").arg(AD_NAME).arg(unique_ids.at(1)).toUtf8().data());
		if (res->next()) {
			pokemon_base *enemy = res_to_pokemon(res);
			pokemon_base *source = player.find_pok_by_unique(unique_ids.at(0).toInt());
			str += QString::fromStdString(source->battle_with(enemy));
			int win_flag = str.split("****").at(1).split("###").at(0).toInt();
			update_sql_battle_num(win_flag);
			if (win_flag) {
				QString pokemon_str = QString("add_pokemon****") + QString::fromStdString(enemy->out_pokemon_info());
				emit string_to_socket_ready(pokemon_str, 1);
				player.put_pokemon_in_store(enemy);
				insert_pok_to_sql(enemy);
				str += "****1";
			}
			else {
				vector<int> three_unique = player.three_unique_id();
				str += QString("****%1,%2,%3").arg(three_unique[0]).arg(three_unique[1]).arg(three_unique[2]);
				if (three_unique[1] == -1) {
					no_pokemon = 1;
				}
			}
			check_skill_update(source);
			update_pokemon_sql();
			emit string_to_socket_ready(str, 1);

		}
	}
	else if (mode == "send_pok") {
		int unique_ = list.at(1).toInt();
		stmt->execute(QString("delete from pokemon_user where user_name='%1' and id_unique=%2\0").arg(user_name).arg(unique_).toUtf8().data());
		player.delet_pok(unique_);
		if (no_pokemon) {
			QString str_ = ":";
			no_pokemon = 0;
			pokemon_base* pok= give_player_random_r(str_);
			emit string_to_socket_ready("information****您已送出全部精灵，送您随机精灵" + str_ + "。", 1);
			QString pokemon_str = QString("add_pokemon****") + QString::fromStdString(pok->out_pokemon_info());
			emit string_to_socket_ready(pokemon_str, 1);
		}


	}
	else if (mode == "fresh_player_info") {
		fresh_player_madels();
		QString res_str = get_player_sql_info(user_name);
		emit string_to_socket_ready(res_str, 1);
	}
}