cmake_minimum_required(VERSION 3.14)
project(Lab_2_2_0 C)

set(CMAKE_C_STANDARD 11)

add_executable(Lab_2_2_0
        main.c)

add_executable(Lab_2_2_2_signals
        main_signals.c)

add_executable(shared_var shared_var.c)

add_executable(t_flock t_flock.c)

add_executable(Lab_2_2_A main_flock.c)
