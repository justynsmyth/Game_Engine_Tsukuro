#include "GameManager.h"
#include <iostream>
#include <sstream>


GameManager::GameManager(int initialHealth, int initialScore) : health(initialHealth), score(initialScore), playerWin(false) {
    state = 'i'; // intro
}


void GameManager::changeHealth(int val) {
    health += val;
    if (health <= 0) {
        setState('e');
    }
}

void GameManager::changeScore(int val) {
    score += val;
}

bool GameManager::running() {
    return run;
}

char GameManager::getState() {
    return state;
}

void GameManager::setState(char c) {
    state = c;
}

int GameManager::getScore() {
    return score;
}

int GameManager::getHealth() {
    return health;
}

bool GameManager::playerWon() {
    return playerWin;
}

void GameManager::setWin(bool state) {
    playerWin = state;
}

void GameManager::quitGame() {
    run = false;
}