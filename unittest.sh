#!/bin/bash

gcc ./test_io.c -o test

./test < "$1" > "$2"

var =$(diff "$1" "$2")

if [ $var != '="" ]; then
    echo "le test sans option a échoué: $var "
fi

./test -t < "$1" > "$2"

var =$(diff "$1" "$2")

if [ $var != '="" ]; then
    echo "le test avec l'option -t a échoué: $var "
fi

./test -p < "$1" > "$2"

var =$(diff "$1" "$2")

if [ $var != '="" ]; then
    echo "le test avec l'option -p a échoué: $var "
fi

./test -o < "$1" > "$2"

var =$(diff "$1" "$2")

if [ $var != '="" ]; then
    echo "le test avec l'option -o a échoué: $var "
fi


