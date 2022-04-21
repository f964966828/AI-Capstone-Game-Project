#include "STcpClient.h"
#include <stdlib.h>
#include <iostream>
#include <queue>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <math.h>

using namespace std;

/*
Team name: qva-sbji-uwd
Team ID: 11
Team member:
    1. 0816124 林浩君
    2. 0811510 許承壹
    3. 0816128 王暐承
*/

vector<vector<int>> get_inital_pos(int mapStat[12][12]);
vector<vector<int>> get_player_pos(int playerID, int mapStat[12][12]);
vector<vector<int>> get_steps(int playerID, int mapStat[12][12], int sheepStat[12][12]);
vector<int> move_one_step(int x, int y, int dir);
vector<int> move_to_end(int x, int y, int dir, int mapStat[12][12]);
int calculate_score(int playerID, int mapStat[12][12]);

float calculate_block_score(vector<int> pos, int mapStat[12][12], int layer);
vector<int> playout(int playerID, int mapStat[12][12], int sheepStat[12][12], bool player_end[4]);
vector<int> take_step(vector<int> step, int playerID, bool player_end[4], int mapStat[12][12], int sheepStat[12][12]);
int evaluate_scores(int playerID, vector<int> scores);

/*
    選擇起始位置
    選擇範圍僅限場地邊緣(至少一個方向為牆)
    
    return: init_pos
    init_pos=<x,y>,代表你要選擇的起始位置
    
*/
vector<int> InitPos(int mapStat[12][12]){
	vector<int> init_pos(2);

	/*
		Write your code here
	*/

	vector<vector<int>> inital_pos = get_inital_pos(mapStat);
	float max_block_score = 0.0;
	for(auto pos: inital_pos){
		float block_score = calculate_block_score(pos, mapStat, 1);
		if(max_block_score < block_score){
			max_block_score = block_score;
			init_pos = pos;
		}
	}
    
    return init_pos;
}

/*
	產出指令
    
    input: 
	playerID: 你在此局遊戲中的角色(1~4)
    mapStat : 棋盤狀態, 為 12*12矩陣, 
					0=可移動區域, -1=障礙, 1~4為玩家1~4佔領區域
    sheepStat : 羊群分布狀態, 範圍在0~16, 為 12*12矩陣

    return Step
    Step : <x,y,m,dir> 
            x, y 表示要進行動作的座標 
            m = 要切割成第二群的羊群數量
            dir = 移動方向(1~6),對應方向如下圖所示
              1  2
            3  x  4
              5  6
*/

vector<int> GetStep(int playerID, int mapStat[12][12], int sheepStat[12][12]){

	vector<int> step;
	step.resize(4);
	time_t start_time = time(NULL);

	/*
		Write your code here
	*/
    
	vector<vector<int>> steps = get_steps(playerID, mapStat, sheepStat);
	vector<pair<float, float>> UCB1(steps.size()); // first: w, second: n
	for(int i=0; i<steps.size(); i++){
		UCB1[i].first = 0;
		UCB1[i].second = 0;
	}

	// take all action 1 times
	for(int i=0; i<steps.size(); i++){
		vector<int> step = steps[i];
		bool player_end[4] = {false, false, false, false};

		vector<int> scores  = take_step(step, playerID, player_end, mapStat, sheepStat);

		UCB1[i].second += 3;
		UCB1[i].first += evaluate_scores(playerID, scores);
	}

	// run playout until time limit
	int t = steps.size() * 3;
	while(time(NULL) - start_time < 5){
		float max_UCB1_score = 0.0;
		int step_id = 0;
		for(int id=0; id<UCB1.size(); id++){
			float UCB1_score = UCB1[id].first / UCB1[id].second + sqrt(2 * log(t) / UCB1[id].second);
			if(max_UCB1_score < UCB1_score){
				max_UCB1_score = UCB1_score;
				step_id = id;
			}
		}

		vector<int> step = steps[step_id];
		bool player_end[4] = {false, false, false, false};

		vector<int> scores = take_step(step, playerID, player_end, mapStat, sheepStat);

		UCB1[step_id].second += 3;
		UCB1[step_id].first += evaluate_scores(playerID, scores);
		t += 3;
	}

	// choose the step with maximum smaple number
	int step_id = 0, max_n = 0;
	for(int i=0; i<UCB1.size(); i++){
		if(max_n < UCB1[i].second){
			max_n = UCB1[i].second;
			step_id = i;
		}
	}

	step = steps[step_id];
    return step;
}

int main(){
	srand(time(NULL));

	int id_package;
	int playerID;
    int mapStat[12][12];
    int sheepStat[12][12];

	// player initial
	GetMap(id_package,playerID,mapStat);
	std::vector<int> init_pos = InitPos(mapStat);
	SendInitPos(id_package,init_pos);

	while (true)
	{
		if (GetBoard(id_package, mapStat, sheepStat))
			break;

		std::vector<int> step = GetStep(playerID,mapStat,sheepStat);
		SendStep(id_package, step);
	}
}

vector<vector<int>> get_inital_pos(int mapStat[12][12]){
	vector<vector<int>> initial_pos;
	for(int x=0; x<12; x++){
		for(int y=0; y<12; y++){
			if(mapStat[x][y] == 0){
				bool flag = false;
				for(int dir=1; dir<=6; dir++){
					vector<int> next_pos = move_one_step(x, y, dir);
					int nx = next_pos[0], ny = next_pos[1];
					if(nx == -1 && ny == -1) continue;
					if(mapStat[nx][ny] == -1) flag = true;
				}
				if(flag){
					vector<int> pos(2);
					pos[0] = x;
					pos[1] = y;
					initial_pos.push_back(pos);
				}
			}
		}
	}
	return initial_pos;
}

vector<vector<int>> get_player_pos(int playerID, int mapStat[12][12]){
	vector<vector<int>> player_pos;
	for(int i=0; i<12; i++){
		for(int j=0; j<12; j++){
			if(mapStat[i][j] == playerID){
				vector<int> pos(2);
				pos[0] = i;
				pos[1] = j;
				player_pos.push_back(pos);
 			}
		}
	}
	return player_pos;
}

vector<vector<int>> get_steps(int playerID, int mapStat[12][12], int sheepStat[12][12]){
	vector<vector<int>> player_pos = get_player_pos(playerID, mapStat);
	vector<vector<int>> steps;
	vector<int> step(4);
	for(auto pos: player_pos){
		int x = pos[0], y = pos[1];
		if(sheepStat[x][y] == 1) continue;
		for(int dir=1; dir<=6; dir++){
			vector<int> end_pos = move_to_end(x, y, dir, mapStat);
			int nx = end_pos[0], ny = end_pos[1];

			if(x == nx && y == ny) continue;

			for(int m=1; m<sheepStat[x][y]; m++){
				step[0] = x;
				step[1] = y;
				step[2] = m;
				step[3] = dir;
				steps.push_back(step);
			}
		}
	}
	return steps;
}

vector<int> move_one_step(int x, int y, int dir){
	vector<int> next_pos(2);

	int dx_odd[] = {0, 1, -1, 1, 0, 1};
	int dx_even[] = {-1, 0, -1, 1, -1, 0};
	int dy[] = {-1, -1, 0, 0, 1, 1};

	int nx = x + (y%2 ? dx_odd[dir-1] : dx_even[dir-1]);
	int ny = y + dy[dir-1];

	if(nx < 0 || nx > 11 || ny < 0 || ny > 11){
		nx = -1;
		ny = -1;
	}

	next_pos[0] = nx;
	next_pos[1] = ny;
	return next_pos;
}

vector<int> move_to_end(int x, int y, int dir, int mapStat[12][12]){
	vector<int> next_pos = move_one_step(x, y, dir);
	int nx = next_pos[0], ny = next_pos[1];
	if((nx == -1 && ny == -1) || mapStat[nx][ny] != 0){
		std:vector<int> pos(2);
		pos[0] = x;
		pos[1] = y;
		return pos;
	}else return move_to_end(nx, ny, dir, mapStat);
}

int calculate_score(int playerID, int mapStat[12][12]){
	vector<vector<int>> player_pos = get_player_pos(playerID, mapStat);
	vector<vector<int>> visited(12);
	for(int i=0; i<12; i++) visited[i].resize(12);

	//BFS find clusters
	vector<vector<vector<int>>> clusters;
	for(auto pos: player_pos){
		int x = pos[0], y = pos[1];
		if(visited[x][y]) continue;
		vector<vector<int>> cluster;
		cluster.push_back(pos);
		queue<vector<int>> q;
		q.push(pos);
		while(!q.empty()){
			x = q.front()[0]; y = q.front()[1]; q.pop();
			visited[x][y] = 1;
			for(int dir=1; dir<=6; dir++){
				vector<int> next_pos = move_one_step(x, y, dir);
				int nx = next_pos[0], ny = next_pos[1];
				if(nx != -1 && ny != -1 && !visited[nx][ny] && mapStat[nx][ny] == playerID){
					cluster.push_back(next_pos);
					q.push(next_pos);
					visited[nx][ny] = 1;
				}
			}
		}
		clusters.push_back(cluster);
	}

	int max_len = 0;
	for(auto cluster: clusters) max_len = max(max_len, int(cluster.size()));
	
	return player_pos.size() * 3 + max_len;
}

float calculate_block_score(vector<int> pos, int mapStat[12][12], int layer){
	if(layer > 4) return 0;
	
	float block_score = 0.0;
	for(int dir=1; dir<=6; dir++){
		int x = pos[0], y = pos[1];
		vector<int> next_pos = move_to_end(x, y, dir, mapStat);
		int nx = next_pos[0], ny = next_pos[1];
		if(x != nx || y != ny){
			mapStat[x][y] = -1;
			block_score += 1.0 + 0.1 * calculate_block_score(next_pos, mapStat, layer+1);
			mapStat[x][y] = 0;
		}
	}
	return block_score;
}

// return scores
vector<int> playout(int playerID, int mapStat[12][12], int sheepStat[12][12], bool player_end[4]){
	bool flag = true; // all players end
	for(int i=0; i<4; i++) flag = flag && player_end[i];
	if(flag){
		vector<int> scores;
		for(int id=1; id<=4; id++){ 
			scores.push_back(calculate_score(id, mapStat));
		}
		return scores;
	}else{
		// swich to next player
		if(player_end[playerID-1]) return playout(playerID%4+1, mapStat, sheepStat, player_end);

		vector<vector<int>> steps = get_steps(playerID, mapStat, sheepStat);
		// no action to take
		if(steps.size() == 0){
			player_end[playerID-1] = true;
			return playout(playerID%4+1, mapStat, sheepStat, player_end);
		}else{
			int idx = rand() % steps.size();
			vector<int> step = steps[idx]; // <x,y,m,dir> 
			
			return take_step(step, playerID, player_end, mapStat, sheepStat);
		}
	}
}

vector<int> take_step(vector<int> step, int playerID, bool player_end[4], int mapStat[12][12], int sheepStat[12][12]){
	int x = step[0], y = step[1], m = step[2], dir = step[3];
	vector<int> next_pos = move_to_end(x, y, dir, mapStat);
	int nx = next_pos[0], ny = next_pos[1];

	mapStat[nx][ny] = playerID;
	sheepStat[nx][ny] = m;
	sheepStat[x][y] -= m;
	
	vector<int> scores = playout(playerID%4+1, mapStat, sheepStat, player_end);

	mapStat[nx][ny] = 0;
	sheepStat[nx][ny] = 0;
	sheepStat[x][y] += m;

	return scores;
}

int evaluate_scores(int playerID, vector<int> scores){
	int score = 3;
	for(int id=1; id<=4; id++){
		if(scores[id-1] > scores[playerID-1]) score--;
	}
	return score;
}