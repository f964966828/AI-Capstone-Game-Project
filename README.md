# Battle Sheep

## Introduction
- This project need to design a game-playing agent to play a **board game** called Battle Sheep, this is a turn-taking(4 players), deterministic, perfect-information game.

<img src="https://i.imgur.com/uQy3McF.png" alt="drawing" style="width:500px;"/>

## Implementation

### Initial Position
- When choose initial position, we calculate the **cell_score** for every possible initial cell, means how good to choose this cell, and the calculation of cell_score is:

<div align="center">
<img src="https://render.githubusercontent.com/render/math?math=\text{cell score} = \sum_{i=1}^{4} \gamma^i n_i"/>
</div>

- where <img src="https://render.githubusercontent.com/render/math?math=n_i"/> is the total possible cell number that we can reach after <img src="https://render.githubusercontent.com/render/math?math=i"/> steps, and <img src="https://render.githubusercontent.com/render/math?math=\gamma"/> as the discount factor, here we choose <img src="https://render.githubusercontent.com/render/math?math=\gamma=0.1"/>
- then we choose the cell with the maximum cell_score as our initial position

### Get Step
- When choose the step in each round, we implement **Monte Carlo Tree Search**(MCTS) algorithm, firstly find all possible step then evaluate each step using UCB1 formula

<div align="center">
<img src="https://render.githubusercontent.com/render/math?math=UCB1(step_i) = \frac{w_i}{n_i} + \sqrt{\frac{2ln(t)}{n_i}}"/>
</div>
        
- Where
    - <img src="https://render.githubusercontent.com/render/math?math=w_i"/> stands for the number of wins of this step
    - <img src="https://render.githubusercontent.com/render/math?math=n_i"/> stands for the number of simulations of this step
    - <img src="https://render.githubusercontent.com/render/math?math=t"/> stands for the total number of simulations, equal to the summantion of <img src="https://render.githubusercontent.com/render/math?math=n_i"/>
- But we soon find a potential problem, since the formula above only get reward when our agent win, if there's another player that will win definitly, which means our agent has no chance to get 1st place, our agent will do random action since UCB1 score for every steps are all equal.
- Consider it's a 4-players game, we cannot only reward 1st place but also 2nd and 3rd, so i change the definition of <img src="https://render.githubusercontent.com/render/math?math=w_i"/>
    - <img src="https://render.githubusercontent.com/render/math?math=w_i"/> = the total win_score of this step
    - where win_score = [1, 2/3, 1/3, 0] when we get 1st, 2nd, 3rd and 4th place
- In this design, we can avoid the problem above, whether our agent is possible to get 1st place it will try its best to get higher place, of course the win_score not must to be choose linearly, you can design your own win_score.

# Pacman

## Introduction
- The objective of this assignment to design a game-playing agent. The game to be played is a modified version of multi-player Pacman game and decisions have to be made in **real time**.

<img src="https://i.imgur.com/LeMjmtG.png" alt="drawing" style="width:500px;"/>

## Implementation

### Decide Direction
- Firstly, produce a 16x16 grid to help us know the information of map
    - -2: ghost position
    - -1: position that ghost can reach in 2 steps 
    - 0: empty block
    - 1: unactivate landmines
    - 2: power pellets
	- 3: pellets
	- 4: activate landmines
- Then, set priority and avoid list that our agent should find and should avoid
    - If agent in power state
        - priority = {{-2, 2}, {1, 3}}
        - avoid = {4}
    - If agent not in power state
        - priority = {{2}, {1, 3}}
        - avoid = {-1, -2, 4}
- Priority = {{2}, {1, 3}} for example, means power pellets has the first priority as our target, and if there's has no power pellets, then we find pellets and unactivate landmines instead, and these two things has the same priority.
- Using **Breadth-First Search** (BFS) as our implementation algorithm, for each possible direction, calculate the neareast distance for things in priority define above, then take direction with the highest priority and the neareast distance.
- When we meet things in avoid list, just take it as a wall and don't take this direction.
    
### Throw Landmines
- Simply take random action since i have know idea how to evaluate a good throw
