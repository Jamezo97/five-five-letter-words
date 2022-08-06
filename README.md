# Five Five-Letter Words with 25 Unique Characters

Inspired by [Matt Parker's Youtube video](https://youtu.be/_-AfhLQfb6w)

My own implementation of the 5-clique solution, inspired by [Benjamin Paassen](https://gitlab.com/bpaassen/five_clique)

Wanted to see how fast I could make the algorithm. When built in release mode, on a single thread, it takes about 80 seconds on my computer.

## Algorithm
I've taken advantage of the fact that 25 characters can be represented as single bits inside a 32 bit integer.
For every word, we build its bit representation. This also helps filter out anagrams as they have the same bit representation.

```
00000000 00000000 00000000 00000000
------zy xwvutsrq ponmlkji hgfedcba

// e.g. ambry (0x01021003  /  16912387)
00000001 00000010 00010000 00000011
------zy xwvutsrq ponmlkji hgfedcba
```

To find if a word is only comprised of unique characters, we scan through each letter and check if the bit was set by a previous character.

To determine if two words have any overlap, we simply bit-wise AND the two hash's and check if it equals zero.

Anagrams of the same word will have the same bit representation. (e.g. ambry and barmy).

We build a network of 'nodes' which are identified by their bit representation. Each node contains 1 or more actual words, all of which are anagrams.

If two nodes don't share any letters, they are connected together.

We then scan through all nodes, and each node scans its neighbours trying to find a group of 5 words without overlap.

The results are accumulated, duplicates are removed. We then expand each node in each set of 5 combinatorially to get all word combinations.

The result, is 831 unique combinations of 5 words.

## Threading
Can run multithreaded. Specify the number of threads with the `-t<n>` argument. e.g.
`./2_run.sh -t8`

With 8 threads it takes about 25 seconds on my computer.
