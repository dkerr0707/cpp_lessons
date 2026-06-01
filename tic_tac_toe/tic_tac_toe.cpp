#include <iostream>
#include <vector>

// set up the board
// run loop - alternate turns
// win, lose, draw detection

enum class CellState { EMPTY, X, O };
enum class GameState { PLAYABLE, X_WIN, O_WIN, DRAW };

char to_char(CellState c) {

    switch (c) {
        case CellState::X: return 'X';
        case CellState::O: return 'O';
        case CellState::EMPTY: return '.';
    }
    return '?';
}

std::string to_string(GameState s) {

    switch (s) {
        case GameState::PLAYABLE: return "Playable";
        case GameState::X_WIN: return "X Win";
        case GameState::O_WIN: return "O Win";
        case GameState::DRAW: return "Draw";
    }
    return "Invalid";
}

void print(std::vector<CellState> b) {

    std::cout << 
        to_char(b[0]) << ' ' << to_char(b[1]) << ' ' << to_char(b[2]) << '\n' <<
        to_char(b[3]) << ' ' << to_char(b[4]) << ' ' << to_char(b[5]) << '\n' <<
        to_char(b[6]) << ' ' << to_char(b[7]) << ' ' << to_char(b[8]) << '\n';

}

GameState get_game_state(std::vector<CellState> b) {

    static const int lines[8][3] = {
        {0,1,2}, {3,4,5}, {6,7,8}, // rows
        {0,3,6}, {1,4,7}, {2,5,8}, // cols
        {0,4,8}, {2,4,6},          // diagonals
    };

    for (auto& L: lines) {
        if (b[L[0]] != CellState::EMPTY &&
            b[L[0]] == b[L[1]] && b[L[1]] == b[L[2]]) {
            return b[L[0]] == CellState::X ? GameState::X_WIN : GameState::O_WIN;
        }
    }

    for (auto& s: b) {
        if (s == CellState::EMPTY) return GameState::PLAYABLE;
    }

    return GameState::DRAW;
}

bool set_cell(std::vector<CellState>& b, int cell, CellState to_state) {

    if (cell < 0 || cell >= static_cast<int>(b.size())) return false; // invalid index
    if (b[cell] != CellState::EMPTY) return false; // must be empty

    b[cell] = to_state;
    return true;
}

int main() {

    std::vector<CellState> board(9, CellState::EMPTY);
    int cell;

    while (get_game_state(board) == GameState::PLAYABLE) {

        print(board);
        std::cout << to_string(get_game_state(board)) << "\n\n";

        
        std::cout << "X to Play: ";
        std::cin >> cell;
        while (!set_cell(board, cell, CellState::X)) {
            std::cout << "Invalid Selection, try again. " << '\n';
            std::cin >> cell;
        }
        std::cout << '\n';
        
        print(board);
        GameState state = get_game_state(board);
        std::cout << to_string(state) << "\n\n";
        if (state == GameState::X_WIN ||
            state == GameState::DRAW) break;
        

        std::cout << "O to Play: ";
        std:: cin >> cell;
        while (!set_cell(board, cell, CellState::O)) {
            std::cout << "Invalid Selection, try again." << '\n';
            std::cin >> cell;
        }
        std::cout << '\n';

        print(board);
        std::cout << to_string(get_game_state(board)) << "\n\n";
    }

    return EXIT_SUCCESS;
}

