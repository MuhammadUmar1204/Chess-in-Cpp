# Chess-in-C++
A C++ chess game with SFML graphics, local multiplayer &amp; AI opponent. Features full chess logic, mouse control, Unicode piece symbols, turn/check display, and instant restart. Ready to compile with SFML 2.5+.
Chess – Advanced Graphics, Multiplayer & AI
A complete C++ chess game with SFML graphics, local multiplayer, and AI opponent.
Features full chess rules (excluding castling/en passant), smooth mouse-driven interface, and Unicode piece symbols.

# Features
🎨 Advanced graphics – custom board colors, piece highlights, and Unicode chess symbols.

👥 Two game modes – Human vs Human or Human vs AI (AI plays random legal moves).

♟️ Full chess logic – all pieces move correctly, check detection, pawn promotion, checkmate/stalemate.

🖱️ Mouse-controlled – click to select a piece, click again to move.

🎮 Easy restart – press R to reset the game at any time.

# Requirements

C++17 compatible compiler (g++, clang, MSVC)

SFML 2.5+ graphics library

A TrueType font file (e.g., arial.ttf)

Installation & Compilation
1. Install SFML
Linux (Ubuntu/Debian)
bash
sudo apt update
sudo apt install libsfml-dev
macOS (Homebrew)
bash
brew install sfml
Windows (MinGW)
Download the latest SFML release for MinGW from sfml-dev.org.

Extract the archive to a folder, e.g., C:\SFML.

Add the include and lib paths to your compiler (see compilation instructions).

2. Get the Font
Download any TrueType font (e.g., from Google Fonts) and place it in the same folder as the source code.
Rename it to arial.ttf or modify the code to match your font filename.

3. Compile the Code
Save the provided C++ code as chess.cpp.

Linux / macOS
bash
g++ -std=c++17 chess.cpp -o chess -lsfml-graphics -lsfml-window -lsfml-system
Windows (MinGW)
bash
g++ -std=c++17 chess.cpp -o chess.exe -IC:\SFML\include -LC:\SFML\lib -lsfml-graphics -lsfml-window -lsfml-system
After compilation, copy the SFML DLLs from C:\SFML\bin to the same folder as chess.exe.

Running the Game
Run the executable:

Linux/macOS: ./chess

Windows: chess.exe

A menu appears:

Press 1 for Human vs Human.

Press 2 for Human vs AI (AI plays black).

Gameplay:

Click on a piece to select it (highlighted in yellow).

Click on a valid square to move.

The game shows turn status and check warnings at the top.

Press R to restart at any time.

Code Structure
Piece – struct representing a chess piece (type, color, moved flag).

Board – core logic: move validation, check detection, game state.

AI – simple random move generator.

ChessGUI – SFML rendering, event handling, game loop.

Limitations & Future Improvements
❌ Castling and en passant are not implemented (easy to add).

❌ AI is random – could be upgraded with minimax and evaluation.

❌ No network multiplayer – can be added using SFML networking.

❌ No move animations – but can be implemented with smooth transitions.

Feel free to extend and contribute!

License
This project is open source under the MIT License.
