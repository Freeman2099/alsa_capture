for a in {0..9}
do
    sox -t raw -c 1 -e signed-integer -b 16 -r 16000 ch$a.raw ch$a.wav
done
