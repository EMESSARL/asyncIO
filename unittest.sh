#!/bin/bash

gcc ./test_io.c -o test

./test < "$1" > "$2"

var=$(diff "$1" "$2")

<<<<<<< HEAD
if [ $var != "" ]; then
=======
if [ $var !=  "" ]; then
>>>>>>> 3670adcd7aa31c5d85d97f031ebb5f6844e27bbd
    echo "le test sans option a échoué: $var "
fi

echo "OK test sans option"

./test -t < "$1" > "$2"

var=$(diff "$1" "$2")

<<<<<<< HEAD
if [ $var != "" ]; then
=======
if [ $var !=  "" ]; then
>>>>>>> 3670adcd7aa31c5d85d97f031ebb5f6844e27bbd
    echo "le test avec l'option -t a échoué: $var "
fi

echo "OK test avec option -t"

./test -p < "$1" > "$2"

var=$(diff "$1" "$2")

if [ $var != "" ]; then
    echo "le test avec l'option -p a échoué: $var "
fi

echo "OK test avec option -p"

./test -o < "$1" > "$2"

var=$(diff "$1" "$2")

if [ $var != "" ]; then
    echo "le test avec l'option -o a échoué: $var "
fi

echo "OK test avec option -o"

