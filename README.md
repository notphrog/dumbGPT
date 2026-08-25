 # dumbGPT
a high performance, zero dependancy, c++ 20, 5 gram (dynamic), lightweight markov chain text generator that trains on raw txt files you give it in real time on the cpu with katz backoff
a high performance, zero dependency, c++ 20, 5 gram (dynamic), lightweight markov chain text generator that trains on raw .txt files you give it in real time on the cpu with katz backoff


thats cool.. but how do i use it?

let me show you and thanks for asking!

# how to use ⮋
## how to use out of the box


1. unzip the folder and run the .exe
2. type the max n-gram size
3. to use one of the pre packaged datasets, type 'template'
4. type a template name *exactly* with the .txt at the end
5. wait until it says "ai training finished"
6. type the prompt, but keep the word count equal to or under the max n-gram size you set at the beginning
7. turning finish at nearest period means the ai will go to the max word count and past it, stopping at the next punctuation mark (!, ?, .)
8. let the ai run and save the output if you want

<br>

## how to give it custom data

1. download a .txt file, and drag it into the training_data folder
2. copy the filepath
3. run the program and type 'new'
4. paste the filepath of the .txt you downloaded and hit enter (DELETE THE QUOTES AROUND IT)
5. if you downloaded multiple, repeat the process
6. the program can handle no more than 60mb of text at once on a 16gb ram system, and it increases just like you think it does with more
7. when all files are in, say 'done'
8. to save that bundle as a template to use later, say 'yes' when it asks and give a name ending in .txt (e.g. sciencey_template.txt)
9. these templates can now be loaded when you come back later

<br>
<br>

## building from source

if you would like to build the project from source, open the terminal navigate to the project file, and if youre on..

### ..on mac and linux? (with gcc or clang)
run: g++ -O3 -std=c++17 ai_markov.cpp -o ai_markov

then: ./ai_markov
<br>
### ..on windows? (with mingw/gcc)
run: g++ -O3 -std=c++17 ai_markov.cpp -o ai_markov.exe

then: ai_markov.exe
<br>
### ..on windows? (with msvc)
run: cl /EHsc /O2 /std:c++17 ai_markov.cpp

then: ai_markov.exe

<br>
<br>

## thats great and all.. but how does this thing ***actually*** work?

its great you ask!

this project was built with the **c++ stl**! here are some insights..!

<br>

**the main idea:** at its heart, this project builds a dynamic directed graph of word transitions based on whatever text you feed it.

**tokenization & mapping:** the core algorithm breaks input text into word pairs, then it maps an incoming state ($Word_{N-1}$) to a frequency list of all possible next words ($Word_N$)! then it chooses one, and thats the output you see! if youre curious, you can look at what the program is printing while its running

**weighted randomness:** when picking the next word in a sequence, the engine picks based on *frequency*. if a word appears 10 times after "the" in your training data and another word only appears once, the engine is 10x more likely to pick the first one

**katz backoff:** you start by declaring the max n-gram size, but if it runs into a situation where it cant find a matching phrase at the size of the n-gram, it will go down an n-gram instead of just crashing!

**data & training:** instead of hardcoding text files or cramming everything into one massive file, the project uses a modular data structure so you can swap out datasets without touching the actual c++ code, though in early development before i had this, i did use hardcoded text baked into the program

**custom files and reading:** i had to play around with making custom files to store data it will need. when you type in the name of a template, the program will open template_registry.txt and in there is a list of all templates. if what you typed isnt in there, it will ask you to try again. if it is, it will then open the templates folder and search for the matching name. once it finds the name, it gets all the paths listed in there, grabs them, and trains on them all just like you would if you were training multiple files.

  <sub>technically, the template_registry isnt *required* and does make an extra step, but i did it because it allows a way to give errors before diving into folders, it allows me to have a clean list of all valid templates, avoid doing system calls, and makes a single entry point, saving you the hassle of writing the filepath to the template directly every time</sub>

**fast i/o & and low memory overhead:** by relying on standard library data structures, the program goes through multi megabyte text files and builds the transition graph super super fast, and the amount of training data it can take at once is entirely dependent on your ram

**terminal output & visualizer:** the program handles start of sentence capitalization, attaching punctuation, quote and bracket pairing, and automated punctuation formatting so the generated output reads smoothly in the console! it does this by seperating punctuation as its own word and making rules so it attaches itself onto words, as it knows that ! is a valid follower of x word based on the data, so it preemptively puts them together. when stitching the output together, the program checks if this token is a punctuation mark, if so, put it on without a space. if not, do as normal.
