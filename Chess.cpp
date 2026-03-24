#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <cctype>
#include <random>
#include <chrono>

// ---------------------------------------------------------------------
//  Chess constants & helpers
// ---------------------------------------------------------------------
enum class PieceType { None, King, Queen, Rook, Bishop, Knight, Pawn };
enum class Color { White, Black, None };

struct Piece {
    PieceType type = PieceType::None;
    Color color = Color::None;
    bool hasMoved = false;   // for castling (not used here, but reserved)
};

// Convert piece to Unicode symbol (SFML can render these)
std::string pieceToSymbol(Piece p) {
    if (p.type == PieceType::None) return " ";
    // Unicode chess symbols: U+2654..U+265F
    // White: King 0x2654, Queen 0x2655, Rook 0x2656, Bishop 0x2657, Knight 0x2658, Pawn 0x2659
    // Black: King 0x265A, Queen 0x265B, Rook 0x265C, Bishop 0x265D, Knight 0x265E, Pawn 0x265F
    const wchar_t whiteSymbols[] = { L' ', L'♔', L'♕', L'♖', L'♗', L'♘', L'♙' };
    const wchar_t blackSymbols[] = { L' ', L'♚', L'♛', L'♜', L'♝', L'♞', L'♟' };
    if (p.color == Color::White) {
        return std::string(1, (char)whiteSymbols[static_cast<int>(p.type)]);
    } else if (p.color == Color::Black) {
        return std::string(1, (char)blackSymbols[static_cast<int>(p.type)]);
    }
    return "?";
}

// ---------------------------------------------------------------------
//  Board class – manages pieces and move logic
// ---------------------------------------------------------------------
class Board {
public:
    Board() {
        reset();
    }

    void reset() {
        // Clear board
        for (int i = 0; i < 8; ++i)
            for (int j = 0; j < 8; ++j)
                squares[i][j] = Piece();

        // Place pawns
        for (int i = 0; i < 8; ++i) {
            squares[1][i] = {PieceType::Pawn, Color::White};
            squares[6][i] = {PieceType::Pawn, Color::Black};
        }

        // Place major pieces
        auto placeRow = [this](int row, Color color) {
            squares[row][0] = {PieceType::Rook, color};
            squares[row][1] = {PieceType::Knight, color};
            squares[row][2] = {PieceType::Bishop, color};
            squares[row][3] = {PieceType::Queen, color};
            squares[row][4] = {PieceType::King, color};
            squares[row][5] = {PieceType::Bishop, color};
            squares[row][6] = {PieceType::Knight, color};
            squares[row][7] = {PieceType::Rook, color};
        };
        placeRow(0, Color::White);
        placeRow(7, Color::Black);

        // Reset move flags
        for (auto& row : squares)
            for (auto& p : row)
                p.hasMoved = false;

        // Set turn and check status
        turn = Color::White;
        gameOver = false;
        winner = Color::None;
    }

    // Return piece at (row, col) – row 0 is white back rank
    Piece getPiece(int row, int col) const {
        if (row < 0 || row > 7 || col < 0 || col > 7) return Piece();
        return squares[row][col];
    }

    // Move piece if legal; returns true if move executed
    bool movePiece(int fromRow, int fromCol, int toRow, int toCol) {
        Piece piece = getPiece(fromRow, fromCol);
        if (piece.type == PieceType::None) return false;
        if (piece.color != turn) return false;

        // Check if move is legal according to piece rules
        if (!isLegalMove(fromRow, fromCol, toRow, toCol, piece))
            return false;

        // Make the move on a copy to test for self-check
        Board temp = *this;
        temp.squares[toRow][toCol] = temp.squares[fromRow][fromCol];
        temp.squares[fromRow][fromCol] = Piece();
        if (temp.isCheck(turn)) {
            return false; // moving into check is illegal
        }

        // Actually perform move
        squares[toRow][toCol] = squares[fromRow][fromCol];
        squares[fromRow][fromCol] = Piece();
        squares[toRow][toCol].hasMoved = true;

        // Pawn promotion: if pawn reaches last rank, promote to queen
        if (squares[toRow][toCol].type == PieceType::Pawn) {
            if ((toRow == 0 && turn == Color::Black) || (toRow == 7 && turn == Color::White)) {
                squares[toRow][toCol].type = PieceType::Queen;
            }
        }

        // Switch turn
        turn = (turn == Color::White) ? Color::Black : Color::White;

        // Check if game ended
        if (isCheckmate(turn)) {
            gameOver = true;
            winner = (turn == Color::White) ? Color::Black : Color::White;
        } else if (isStalemate(turn)) {
            gameOver = true;
            winner = Color::None; // draw
        }
        return true;
    }

    Color getTurn() const { return turn; }
    bool isGameOver() const { return gameOver; }
    Color getWinner() const { return winner; }

    // Get all legal moves for a given color (used by AI)
    std::vector<std::tuple<int,int,int,int>> getAllLegalMoves(Color color) const {
        std::vector<std::tuple<int,int,int,int>> moves;
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                Piece p = squares[r][c];
                if (p.color == color) {
                    for (int tr = 0; tr < 8; ++tr) {
                        for (int tc = 0; tc < 8; ++tc) {
                            if (isLegalMove(r, c, tr, tc, p)) {
                                // Test if move would leave/put own king in check
                                Board temp = *this;
                                temp.squares[tr][tc] = temp.squares[r][c];
                                temp.squares[r][c] = Piece();
                                if (!temp.isCheck(color)) {
                                    moves.emplace_back(r, c, tr, tc);
                                }
                            }
                        }
                    }
                }
            }
        }
        return moves;
    }

private:
    Piece squares[8][8];
    Color turn = Color::White;
    bool gameOver = false;
    Color winner = Color::None;

    // Check if a move is legal by piece movement rules (ignoring check)
    bool isLegalMove(int fromRow, int fromCol, int toRow, int toCol, Piece piece) const {
        if (fromRow == toRow && fromCol == toCol) return false;
        Piece target = getPiece(toRow, toCol);
        if (target.color == piece.color) return false;

        int dr = toRow - fromRow;
        int dc = toCol - fromCol;
        int absDR = std::abs(dr);
        int absDC = std::abs(dc);

        switch (piece.type) {
            case PieceType::King:
                return (absDR <= 1 && absDC <= 1);
            case PieceType::Queen:
                return (dr == 0 || dc == 0 || absDR == absDC) && isClearPath(fromRow, fromCol, toRow, toCol);
            case PieceType::Rook:
                return (dr == 0 || dc == 0) && isClearPath(fromRow, fromCol, toRow, toCol);
            case PieceType::Bishop:
                return (absDR == absDC) && isClearPath(fromRow, fromCol, toRow, toCol);
            case PieceType::Knight:
                return (absDR == 2 && absDC == 1) || (absDR == 1 && absDC == 2);
            case PieceType::Pawn: {
                int direction = (piece.color == Color::White) ? 1 : -1;
                // One step forward
                if (dc == 0 && dr == direction && target.type == PieceType::None)
                    return true;
                // Two steps from start
                if (dc == 0 && dr == 2*direction && !piece.hasMoved && 
                    target.type == PieceType::None && getPiece(fromRow+direction, fromCol).type == PieceType::None)
                    return true;
                // Diagonal capture
                if (absDC == 1 && dr == direction && target.type != PieceType::None)
                    return true;
                return false;
            }
            default:
                return false;
        }
    }

    // Check if path is clear for sliding pieces
    bool isClearPath(int fromRow, int fromCol, int toRow, int toCol) const {
        int dr = (toRow - fromRow == 0) ? 0 : (toRow - fromRow) / std::abs(toRow - fromRow);
        int dc = (toCol - fromCol == 0) ? 0 : (toCol - fromCol) / std::abs(toCol - fromCol);
        int r = fromRow + dr, c = fromCol + dc;
        while (r != toRow || c != toCol) {
            if (getPiece(r, c).type != PieceType::None)
                return false;
            r += dr;
            c += dc;
        }
        return true;
    }

    // Check if a specific color is in check
    bool isCheck(Color color) const {
        // Find king position
        int kingRow = -1, kingCol = -1;
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                Piece p = squares[r][c];
                if (p.type == PieceType::King && p.color == color) {
                    kingRow = r; kingCol = c; break;
                }
            }
        }
        if (kingRow == -1) return false; // should never happen

        // See if any opponent piece can capture the king
        Color opponent = (color == Color::White) ? Color::Black : Color::White;
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                Piece p = squares[r][c];
                if (p.color == opponent && isLegalMove(r, c, kingRow, kingCol, p)) {
                    return true;
                }
            }
        }
        return false;
    }

    // Check if the player to move has any legal moves
    bool hasAnyLegalMove(Color color) const {
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                Piece p = squares[r][c];
                if (p.color == color) {
                    for (int tr = 0; tr < 8; ++tr) {
                        for (int tc = 0; tc < 8; ++tc) {
                            if (isLegalMove(r, c, tr, tc, p)) {
                                // Test if move avoids check
                                Board temp = *this;
                                temp.squares[tr][tc] = temp.squares[r][c];
                                temp.squares[r][c] = Piece();
                                if (!temp.isCheck(color)) {
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
        return false;
    }

    bool isCheckmate(Color color) const {
        return isCheck(color) && !hasAnyLegalMove(color);
    }

    bool isStalemate(Color color) const {
        return !isCheck(color) && !hasAnyLegalMove(color);
    }
};

// ---------------------------------------------------------------------
//  Simple AI: selects a random legal move
// ---------------------------------------------------------------------
class AI {
public:
    AI(Color color) : myColor(color) {}

    // Returns (fromRow, fromCol, toRow, toCol)
    std::tuple<int,int,int,int> getMove(const Board& board) {
        auto moves = board.getAllLegalMoves(myColor);
        if (moves.empty()) return { -1, -1, -1, -1 };
        static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        std::uniform_int_distribution<size_t> dist(0, moves.size()-1);
        return moves[dist(rng)];
    }

private:
    Color myColor;
};

// ---------------------------------------------------------------------
//  Graphics using SFML
// ---------------------------------------------------------------------
class ChessGUI {
public:
    ChessGUI() : window(sf::VideoMode(800, 800), "Chess - Advanced Graphics"), board() {
        if (!font.loadFromFile("arial.ttf")) {
            // Fallback: we'll still run, but text may not appear properly
            std::cerr << "Warning: Could not load font arial.ttf. Text may not display.\n";
        }
        // Colors
        lightSquare = sf::Color(240, 217, 181);
        darkSquare = sf::Color(181, 136, 99);
        selectedOutline = sf::Color(255, 255, 0, 128);
        lastMoveOutline = sf::Color(0, 255, 0, 128);

        // Setup piece texts
        for (int i = 0; i < 64; ++i) {
            pieceTexts[i].setFont(font);
            pieceTexts[i].setCharacterSize(70);
            pieceTexts[i].setFillColor(sf::Color::Black);
        }

        // Menu setup
        menuTitle.setFont(font);
        menuTitle.setString("Chess Game");
        menuTitle.setCharacterSize(50);
        menuTitle.setFillColor(sf::Color::White);
        menuTitle.setPosition(300, 200);

        menuItems[0].setFont(font);
        menuItems[0].setString("1. Human vs Human");
        menuItems[0].setCharacterSize(30);
        menuItems[0].setFillColor(sf::Color::White);
        menuItems[0].setPosition(300, 300);

        menuItems[1].setFont(font);
        menuItems[1].setString("2. Human vs AI");
        menuItems[1].setCharacterSize(30);
        menuItems[1].setFillColor(sf::Color::White);
        menuItems[1].setPosition(300, 350);

        statusText.setFont(font);
        statusText.setCharacterSize(24);
        statusText.setFillColor(sf::Color::White);
        statusText.setPosition(10, 10);
    }

    void run() {
        // Show main menu
        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed)
                    window.close();
                if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::Num1) {
                        startGame(false); // Human vs Human
                        return;
                    }
                    if (event.key.code == sf::Keyboard::Num2) {
                        startGame(true); // Human vs AI
                        return;
                    }
                }
            }
            window.clear(sf::Color(50, 50, 50));
            window.draw(menuTitle);
            for (auto& item : menuItems) window.draw(item);
            window.display();
        }
    }

private:
    sf::RenderWindow window;
    Board board;
    AI* ai = nullptr;
    bool vsAI = false;
    Color humanColor = Color::White;   // human plays white if vsAI
    bool waitingForAIMove = false;

    sf::Font font;
    sf::Text pieceTexts[64];
    sf::Text statusText;
    sf::Text menuTitle, menuItems[2];
    sf::Color lightSquare, darkSquare;
    sf::Color selectedOutline, lastMoveOutline;
    int selectedRow = -1, selectedCol = -1;
    int lastFromRow = -1, lastFromCol = -1, lastToRow = -1, lastToCol = -1;

    void startGame(bool withAI) {
        vsAI = withAI;
        if (vsAI) {
            ai = new AI(Color::Black); // AI plays black
            humanColor = Color::White;
        }
        board.reset();
        gameLoop();
    }

    void gameLoop() {
        while (window.isOpen()) {
            // AI move if it's AI's turn
            if (vsAI && !board.isGameOver() && board.getTurn() != humanColor && !waitingForAIMove) {
                waitingForAIMove = true;
                // Simple delay to not block UI (we'll process move next frame)
                // Actually we can compute move immediately; it's fine.
                auto [fr, fc, tr, tc] = ai->getMove(board);
                if (fr != -1) {
                    board.movePiece(fr, fc, tr, tc);
                    lastFromRow = fr; lastFromCol = fc;
                    lastToRow = tr; lastToCol = tc;
                }
                waitingForAIMove = false;
            }

            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed)
                    window.close();

                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    if (!board.isGameOver()) {
                        // Check if it's human's turn
                        if (!vsAI || board.getTurn() == humanColor) {
                            int col = event.mouseButton.x / 100;
                            int row = 7 - (event.mouseButton.y / 100); // because y=0 top, row 0 is white back rank
                            if (row >= 0 && row < 8 && col >= 0 && col < 8) {
                                handleClick(row, col);
                            }
                        }
                    }
                }

                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R) {
                    // Reset game
                    board.reset();
                    selectedRow = selectedCol = -1;
                    lastFromRow = lastFromCol = lastToRow = lastToCol = -1;
                }
            }

            // Render
            window.clear();
            drawBoard();
            drawPieces();
            drawStatus();
            window.display();
        }
    }

    void handleClick(int row, int col) {
        if (selectedRow == -1) {
            // Select piece if it belongs to current player
            Piece p = board.getPiece(row, col);
            if (p.color == board.getTurn()) {
                selectedRow = row;
                selectedCol = col;
            }
        } else {
            // Try to move
            if (board.movePiece(selectedRow, selectedCol, row, col)) {
                lastFromRow = selectedRow; lastFromCol = selectedCol;
                lastToRow = row; lastToCol = col;
            }
            selectedRow = selectedCol = -1;
        }
    }

    void drawBoard() {
        const int squareSize = 100;
        for (int row = 0; row < 8; ++row) {
            for (int col = 0; col < 8; ++col) {
                sf::RectangleShape rect(sf::Vector2f(squareSize-1, squareSize-1));
                rect.setPosition(col * squareSize, (7 - row) * squareSize);
                rect.setFillColor((row + col) % 2 == 0 ? lightSquare : darkSquare);
                window.draw(rect);
            }
        }
        // Draw selection highlight
        if (selectedRow != -1) {
            sf::RectangleShape rect(sf::Vector2f(100, 100));
            rect.setPosition(selectedCol * 100, (7 - selectedRow) * 100);
            rect.setFillColor(selectedOutline);
            window.draw(rect);
        }
        // Draw last move highlight
        if (lastFromRow != -1) {
            sf::RectangleShape rect(sf::Vector2f(100, 100));
            rect.setPosition(lastFromCol * 100, (7 - lastFromRow) * 100);
            rect.setFillColor(lastMoveOutline);
            window.draw(rect);
            rect.setPosition(lastToCol * 100, (7 - lastToRow) * 100);
            window.draw(rect);
        }
    }

    void drawPieces() {
        const int squareSize = 100;
        for (int row = 0; row < 8; ++row) {
            for (int col = 0; col < 8; ++col) {
                Piece p = board.getPiece(row, col);
                if (p.type != PieceType::None) {
                    std::string symbol = pieceToSymbol(p);
                    sf::Text text;
                    text.setFont(font);
                    text.setString(symbol);
                    text.setCharacterSize(70);
                    // Center the symbol in the square
                    sf::FloatRect bounds = text.getLocalBounds();
                    text.setOrigin(bounds.left + bounds.width/2.0f, bounds.top + bounds.height/2.0f);
                    text.setPosition(col * squareSize + squareSize/2.0f, (7 - row) * squareSize + squareSize/2.0f);
                    text.setFillColor(p.color == Color::White ? sf::Color::White : sf::Color::Black);
                    window.draw(text);
                }
            }
        }
    }

    void drawStatus() {
        std::string msg;
        if (board.isGameOver()) {
            Color w = board.getWinner();
            if (w == Color::White) msg = "White wins! Press R to restart.";
            else if (w == Color::Black) msg = "Black wins! Press R to restart.";
            else msg = "Stalemate! Press R to restart.";
        } else {
            msg = (board.getTurn() == Color::White ? "White's turn" : "Black's turn");
            if (board.isCheck(board.getTurn())) msg += " (CHECK!)";
        }
        statusText.setString(msg);
        window.draw(statusText);
    }
};

// ---------------------------------------------------------------------
//  Main entry point
// ---------------------------------------------------------------------
int main() {
    ChessGUI gui;
    gui.run();
    return 0;
}