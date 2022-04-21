#include "STcpClient.h"
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <thread>
#include <windows.h>

std::vector<int> step;

int counter[4] = {1, 0, 3, 2};
int direction[4][2] = { {-1, 0}, {1, 0}, {0, -1}, {0, 1} };

class Mythread{
public:
	bool killed = false;
	int parallel_wall[16][17];
	int vertical_wall[17][16];
	int playerStat[5];
	int otherPlayerStat[3][5];
	int ghostStat[4][2];
	std::vector<std::vector<int>> propsStat;

	int grid[16][16];
	int visited[16][16];
	int prev_playerStat[5];
	int prev_ghostStat[4][2];

	int wall[4];
	int prev_dir;
	int ox, oy, px, py, x, y, nx, ny;

	struct info{
		int x;
		int y;
		int prev_dir;
	};

	/*
	輪到此程式移動

	change Step
	Step : vector, Step = {dir, is_throwLandmine}
			dir= 0: left, 1:right, 2: up, 3: down 4:do nothing
			is_throwLandmine= True/False
	*/
	void GetStep(){		

		make_grid();

		std::vector<int> dirs = get_dirs(playerStat, prev_playerStat);
		int best_dir = select_best_dir(playerStat, dirs);

		int temp_step[2] = {0};
		temp_step[0] = best_dir;
		temp_step[1] = (playerStat[2] > 0);
		if (killed == false){
			step.resize(2);
			step[0] = temp_step[0];
			step[1] = temp_step[1];
		}
		delete this;
	}

	std::vector<int> get_dirs(int stat[2], int prevStat[2]){
		ox = stat[0];
		oy = stat[1];
		px = prevStat[0];
		py = prevStat[1];
		x = ox/25;
		y = oy/25;

		wall[0] = vertical_wall[x][y];
		wall[1] = vertical_wall[x+1][y];
		wall[2] = parallel_wall[x][y];
		wall[3] = parallel_wall[x][y+1];

		if(ox - px < 0) prev_dir = 0;
		if(ox - px > 0) prev_dir = 1;
		if(oy - py < 0) prev_dir = 2;
		if(oy - py > 0) prev_dir = 3;

		std::vector<int> dirs;
		for(int i=0; i<4; i++) if(!wall[i] && counter[prev_dir] != i) dirs.push_back(i);
		return dirs;
	}

	std::vector<int> get_dirs(int x, int y, int prev_dir){
		wall[0] = vertical_wall[x][y];
		wall[1] = vertical_wall[x+1][y];
		wall[2] = parallel_wall[x][y];
		wall[3] = parallel_wall[x][y+1];

		std::vector<int> dirs;
		for(int i=0; i<4; i++) if(!wall[i] && counter[prev_dir] != i) dirs.push_back(i);
		return dirs;
	}

	/*
	grid definition
		-2: ghost center
		-1: around ghost
		0: empty
		1: landmines
		2: power pellets
		3: pellets
		4: bombs
	*/
	void make_grid(){
		for(int i=0; i<16; i++) for(int j=0; j<16; j++) grid[i][j] = 0;
		for(int i=0; i<propsStat.size(); i++){
			int type = propsStat[i][0];
			int x = propsStat[i][1]/25;
			int y = propsStat[i][2]/25;
			grid[x][y] = type + 1;
		}

		std::vector<info> nodes;
		for(int i=0; i<16; i++) for(int j=0; j<16; j++) visited[i][j] = 0;
		for(int i=0; i<4; i++){
			get_dirs(ghostStat[i], prev_ghostStat[i]);
			if((x == 7 || x == 8) && (y == 7 || y == 8)) continue;

			grid[x][y] = -2;
			visited[x][y] = 1;

			info node{.x = x, .y = y, .prev_dir = prev_dir};
			nodes.push_back(node);
		}
		ghost_bfs(nodes, 0);
	}

	void ghost_bfs(std::vector<info> nodes, int dis){
		if(dis == 2) return;
		std::vector<info> next_nodes;
		for(auto node: nodes){
			x = node.x; y = node.y; prev_dir = node.prev_dir;
			std::vector<int> dirs = get_dirs(x, y, prev_dir);

			for(auto dir: dirs){
				nx = x + direction[dir][0];
				ny = y + direction[dir][1];
				if(visited[nx][ny]) continue;

				visited[nx][ny] = 1;
				grid[nx][ny] = -1;

				info node{.x = nx, .y = ny, .prev_dir = dir};
				next_nodes.push_back(node);
			}
		}
		ghost_bfs(next_nodes, dis+1);
	}

	int select_best_dir(int playerStat[5], std::vector<int> dirs){
		int power = (playerStat[3] > 2000);
		std::vector<std::vector<int>> prior;
		std::vector<int> avoid;
		if(power){
			prior.push_back(std::vector<int> {-2, 2});
			prior.push_back(std::vector<int> {1, 3});
			avoid.push_back(4);
		}else{
			prior.push_back(std::vector<int> {2});
			prior.push_back(std::vector<int> {1, 3});
			avoid.push_back(-1);
			avoid.push_back(-2);
			avoid.push_back(4);
		}

		// first: min_dis to prior, second: min_dis to avoid
		std::vector<std::pair<std::vector<int>,int>> scores;
		for(auto dir: dirs){
			nx = playerStat[0]/25 + direction[dir][0];
			ny = playerStat[1]/25 + direction[dir][1];
			
			for(int i=0; i<16; i++) for(int j=0; j<16; j++) visited[i][j] = 0;
			visited[nx][ny] = 1;

			std::vector<info> nodes;
			info node{.x = nx, .y = ny, .prev_dir = dir};
			nodes.push_back(node);

			std::vector<int> prior_score;
			for(auto target: prior){
				for(int i=0; i<16; i++) for(int j=0; j<16; j++) visited[i][j] = 0;
				visited[nx][ny] = 1;
				prior_score.push_back(target_bfs(nodes, target, avoid, 0));
			}

			for(int i=0; i<16; i++) for(int j=0; j<16; j++) visited[i][j] = 0;
			visited[nx][ny] = 1;
			int avoid_score = avoid_bfs(nodes, avoid, 0);

			scores.push_back({prior_score, avoid_score});
		}

		bool find = false;
		int min_dis = 1e9, max_dis = 0, best_dir = dirs[0];
		for(int i=0; i<prior.size(); i++){
			if(find) break;
			for(int j=0; j<scores.size(); j++){
				if(min_dis > scores[j].first[i]){
					min_dis = scores[j].first[i];
					best_dir = dirs[j];
					find = true;
				}
			}
		}

		if(!find){
			for(int j=0; j<scores.size(); j++){
				if(max_dis < scores[j].second){
					max_dis = scores[j].second;
					best_dir = dirs[j];
					find = true;
				}
			}
		}

		return best_dir;
	}

	int target_bfs(std::vector<info> nodes, std::vector<int> target, std::vector<int> avoid, int dis){
		std::vector<info> next_nodes;
		for(auto node: nodes){
			x = node.x; y = node.y; prev_dir = node.prev_dir;
			for(auto it: target) if(grid[x][y] == it) return dis;
			std::vector<int> dirs = get_dirs(x, y, prev_dir);

			for(auto dir: dirs){
				nx = x + direction[dir][0];
				ny = y + direction[dir][1];
				if(visited[nx][ny]) continue;

				bool flag = false;
				for(auto it: avoid) if(grid[nx][ny] == it) flag = true;
				if(flag) continue;

				visited[nx][ny] = 1;

				info node{.x = nx, .y = ny, .prev_dir = dir};
				next_nodes.push_back(node);
			}
		}
		if(next_nodes.size()) return target_bfs(next_nodes, target, avoid, dis+1);
		else return 1e9;
	}

	int avoid_bfs(std::vector<info> nodes, std::vector<int> avoid, int dis){
		std::vector<info> next_nodes;
		for(auto node: nodes){
			x = node.x; y = node.y; prev_dir = node.prev_dir;
			for(auto it: avoid) if(grid[x][y] == it) return dis;
			std::vector<int> dirs = get_dirs(x, y, prev_dir);

			for(auto dir: dirs){
				nx = x + direction[dir][0];
				ny = y + direction[dir][1];
				if(visited[nx][ny]) continue;

				visited[nx][ny] = 1;

				info node{.x = nx, .y = ny, .prev_dir = dir};
				next_nodes.push_back(node);
			}
		}
		if(next_nodes.size()) return avoid_bfs(next_nodes, avoid, dis+1);
		else return 1e9;
	}

	Mythread(int playerStat[5], int ghostStat[4][2], std::vector<std::vector<int>> propsStat, int parallel_wall[16][17], int vertical_wall[17][16]){
		for(int i=0; i<5; i++) this->prev_playerStat[i] = this->playerStat[i];
		for(int i=0; i<5; i++) this->playerStat[i] = playerStat[i];
		for(int i=0; i<4; i++) for(int j=0; j<2; j++) this->prev_ghostStat[i][j] = this->ghostStat[i][j];
		for(int i=0; i<4; i++) for(int j=0; j<2; j++) this->ghostStat[i][j] = ghostStat[i][j];
		for(int i=0; i<16; i++) for(int j=0; j<17; j++) this->parallel_wall[i][j] = parallel_wall[i][j];
		for(int i=0; i<17; i++) for(int j=0; j<16; j++) this->vertical_wall[i][j] = vertical_wall[i][j];
		this->propsStat = propsStat;
		
		std::thread t2(&Mythread::GetStep, this);
		t2.detach();
	}
};

int main(){
	int id_package;
	/*
	playerStat: <x,y,n_landmine,super_time,score>
	otherPlayerStat: 3*<x,y,n_landmine,super_time>
	ghostStat: 4*<[x,y],[x,y],[x,y],[x,y]>
	propsStat: n_props*<type,x,y>
	*/
	int parallel_wall[16][17];
	int vertical_wall[17][16];
	int playerStat[5];
	int otherPlayerStat[3][5];
	int ghostStat[4][2];
	std::vector<std::vector<int>> propsStat;
	// receive map
	if(GetMap(parallel_wall, vertical_wall)){
		// start game
		while(true){
			if(GetGameStat(id_package, playerStat, otherPlayerStat, ghostStat, propsStat)) break;
			step.resize(2);
			step[0] = 5;
			step[1] = 2;
			Mythread *mythread = new Mythread(playerStat, ghostStat, propsStat, parallel_wall, vertical_wall);
			Sleep(40);
			if(step[0] == 5){
				std::cout << "timeout" << std::endl;
				mythread->killed = true;
				step[0] = 4;
				step[1] = 0;
			}
			SendStep(id_package, step);
		}
	}
}
