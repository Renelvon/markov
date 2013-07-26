#! /usr/bin/zsh

make

echo > probs.txt
echo > overflow.txt
echo > meanpack.txt
echo > throughput.txt
echo > meandelay.txt
echo > generations.txt

N=12
let "NK = $N - 1"
lambdaArr=(3 5 12)
mu=10

tolerance=0.05
events_per_gen=2000000
maxgen=1000
verbose=0

for K in {1..$NK}
do
    echo "Running for K = $K..."
    for lambda in $lambdaArr
    do
        echo "\t...and lambda = $lambda"
        ./main $N $lambda $mu $K $events_per_gen $maxgen $tolerance $verbose > out.txt

        echo "lambda = $lambda, K = $K..." >> probs.txt
        grep "Final" out.txt | tr "\t" " " | cut -d' ' -f"3-" >> probs.txt

        echo -n "lambda = $lambda, K = $K..." >> overflow.txt
        grep "Overflow" out.txt >> overflow.txt

        echo -n "lambda = $lambda, K = $K..." >> meanpack.txt
        grep "queue size" out.txt >> meanpack.txt

        echo -n "lambda = $lambda, K = $K..." >> throughput.txt
        grep "Throughput" out.txt >> throughput.txt

        echo -n "lambda = $lambda, K = $K..." >> meandelay.txt
        grep "sojourn" out.txt >> meandelay.txt

        echo -n "lambda = $lambda, K = $K..." >> generations.txt
        grep "Generations simulated" out.txt >> generations.txt
    done

    echo >> probs.txt
    echo >> overflow.txt
    echo >> meanpack.txt
    echo >> throughput.txt
    echo >> meandelay.txt
done

rm out.txt
