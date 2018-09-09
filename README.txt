This directory contains the following:
   * c 
     Directory containing the source files for the Chess C-extension
   * GAMES 
     Directory used by cgiskak.py for saving game information between moves
   * bitmaps
     A directory with the images cgiskak.py uses when representing
     a chess board in HTML

   * ChessLib.txt 
     Opening library
   * PChess.py 
     A python wrapper class which adds functionality to the chess engine 
     implemented in the C-extension
   * README.txt
     *this* file
   * cgiskak.py
   * ClientServerChess.py
     A chess server for playing chess turnaments over a TCP sockets.
     (For benchmarking engines).
   * chess.css & index.html
     HTML documentation
   * makefile
     Calls setup.py
   * setup.py
     Python install script for the C extension

CHANGES:

VERSION 1.0.2
- Kid templates used for HTML/RSS generation instead of vigrid/HTMLWriter
