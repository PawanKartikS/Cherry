cmake_minimum_required(VERSION 3.10)
project(Cherry)

add_executable(cherry args.c ast.c builtin.c eval.c expr.c lex.c list.c main.c
    node.c parse.c symtbl.c token.c util.c)