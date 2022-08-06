
#include <algorithm>
#include <chrono>
#include <fstream>
#include <functional>
#include <iomanip>
#include <ios>
#include <iostream>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <sstream>
#include <stdint.h>
#include <string.h>
#include <string>
#include <vector>

#include <cmath>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

// Words from here https://github.com/dwyl/english-words

constexpr uint32_t CLIQUE_DEPTH = 5;
const std::string INPUT_FILE = "words_alpha.txt";
const std::string OUTPUT_FILE = "cliques_out.csv";

void loadWordList(std::string file, std::vector<std::string> &out, int size)
{
    std::fstream stream(file, std::ios::in);
    if (stream.is_open())
    {
        std::string line;
        while (std::getline(stream, line))
        {
            if (line.size() > 0 && line[line.size() - 1] == '\n')
            {
                line = line.substr(0, line.size() - 1);
            }
            if (line.size() > 0 && line[line.size() - 1] == '\r')
            {
                line = line.substr(0, line.size() - 1);
            }
            if (line.size() == size)
            {
                out.push_back(line);
            }
        }
    }
}

uint32_t wordHash(const std::string &word)
{
    uint32_t hash = 0u;
    for (char c : word)
    {
        int index = c - 'a';
        if (index >= 0 && index < 32)
        {
            const uint32_t bit = 1u << (index);
            hash |= bit;
        }
    }
    return hash;
}

bool areWordCharsAllUnique(const std::string &word)
{
    uint32_t lookup = 0u;
    for (char c : word)
    {
        int index = c - 'a';
        if (index >= 0 && index < 32)
        {
            const uint32_t bit = 1u << (index);
            if (lookup & bit)
            {
                return false;
            }
            else
            {
                lookup |= bit;
            }
        }
        else
        {
            return false;
        }
    }

    return true;
}

class Timer
{
public:
    Timer()
    {
        tick();
    }

    void tick()
    {
        m_start =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    }

    long long tock()
    {
        std::chrono::milliseconds tNow =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
        std::chrono::milliseconds delta = tNow - m_start;
        return delta.count();
    }

    void printTock()
    {
        std::cout << "Took " << tock() << " ms\n";
    }

    std::chrono::milliseconds m_start;
};

template <typename Type, uint32_t _capacity>
struct FixedVector
{
    static constexpr uint32_t capacity = _capacity;
    Type &operator[](uint32_t i)
    {
        return data[i];
    }
    const Type &operator[](uint32_t i) const
    {
        return data[i];
    }
    const auto begin() const
    {
        return &(data[0]);
    }
    const auto end() const
    {
        return &(data[_capacity]);
    }
    auto begin()
    {
        return &(data[0]);
    }
    auto end()
    {
        return &(data[_capacity]);
    }
    Type data[_capacity];
};

using WordList = FixedVector<uint32_t, CLIQUE_DEPTH>;
using StringList = FixedVector<std::string, CLIQUE_DEPTH>;

uint32_t myIntHsh(uint32_t x) {
    // use our own hash, as the implementation differs between windows and linux
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

template <uint32_t _capacity>
struct std::hash<FixedVector<uint32_t, _capacity>>
{
    using _dtype = FixedVector<uint32_t, _capacity>;
    std::size_t operator()(_dtype const &items) const noexcept
    {
        std::size_t seed{0u};
        for (const uint32_t &item : items)
        {
            // Hash the number itself and then xor the seeds, as we don't care about word order
            seed ^= myIntHsh(item);
        }
        return seed;
    }
};

/// Represents a unique set of letters
/// Contains a list of all words which contain this unique set
class Node
{
public:
    Node(uint32_t hash, std::string firstWord) : m_hash(hash)
    {
        m_words.push_back(firstWord);
    }

    void addWord(std::string word)
    {
        m_words.push_back(word);
    }

    bool hasOverlap(const Node &other) const
    {
        return (m_hash & other.m_hash) != 0;
    }

    void connect(Node &other)
    {
        m_edges.push_back(other.m_hash);
        other.m_edges.push_back(m_hash);
    }

    /// Hard coded 5-len clique finder
    /// Fun fact, this is **slower** than the generalized version, this one takes 50% longer
    void find_sequences(std::vector<WordList> &out)
    {
        const uint32_t count = static_cast<uint32_t>(m_edges.size());

        uint32_t hash0{m_hash};

        uint32_t i1, i2, i3, i4;

        for (i1 = 0u; i1 < count - 1; ++i1)
        {
            const uint32_t firstHash = m_edges[i1];
            // Don't need to check first hash as all edges already guarantee no clash with this node
            // if((firstHash & hash0) != 0) continue;
            const uint32_t hash1 = firstHash | hash0;

            for (i2 = i1 + 1; i2 < count; i2++)
            {
                const uint32_t secondHash = m_edges[i2];
                if ((secondHash & hash1) != 0)
                    continue;
                const uint32_t hash2 = secondHash | hash1;

                for (i3 = i2 + 1; i3 < count; i3++)
                {
                    const uint32_t thirdHash = m_edges[i3];
                    if ((thirdHash & hash2) != 0)
                        continue;
                    const uint32_t hash3 = thirdHash | hash2;

                    for (i4 = i3 + 1; i4 < count; i4++)
                    {
                        const uint32_t fourthHash = m_edges[i4];
                        if ((fourthHash & hash3) != 0)
                            continue;
                        WordList output;
                        output[0] = fourthHash;
                        output[1] = thirdHash;
                        output[2] = secondHash;
                        output[3] = firstHash;
                        output[4] = m_hash;
                        out.push_back(output);
                    }
                }
            }
        }
    }

    std::string word() const
    {
        return m_words[0];
    }

    const std::vector<std::string> &getWords() const
    {
        return m_words;
    }

    /// \brief Find `_depth` groups of words with no overlap, including the words represented by this node
    void find_cliques(std::vector<WordList> &out) const
    {
        const uint32_t count = static_cast<uint32_t>(m_edges.size());
        const uint32_t index = WordList::capacity - 1;
        WordList current;
        current[index] = m_hash;
        find_next(out, current, 0, count, m_hash, index - 1);
    }

private:
    // template <uint32_t _depth>
    inline void find_next(std::vector<WordList> &out, WordList &current, uint32_t i0, uint32_t count, uint32_t hash,
                          uint32_t index) const
    {
        for (uint32_t i = i0; i < count - index; ++i)
        {
            const uint32_t nodeHash = m_edges[i];
            // Node has overlap
            if ((nodeHash & hash) != 0)
                continue;
            // Store the new node
            current[index] = nodeHash;
            // If we're the last word to be added
            if (index == 0)
            {
                // Add to result vec
                out.push_back(current);
            }
            else
            {
                // recurse to find the next node
                find_next(out, current, i + 1, count, nodeHash | hash, index - 1);
            }
        }
    }

    const uint32_t m_hash;
    std::vector<uint32_t> m_edges{};
    std::vector<std::string> m_words{};
};

std::string join(StringList words)
{
    std::stringstream ss;
    int i = 0;
    for (int i = 0; i < StringList::capacity; ++i)
    {
        if (i != 0)
        {
            ss << ",";
        }
        ss << words[i];
    }
    return ss.str();
}

void findUniqueWords(const std::vector<std::string> &allWords, std::vector<std::string> &out)
{
    for (const auto word : allWords)
    {
        if (areWordCharsAllUnique(word))
        {
            out.push_back(word);
        }
    }
}

class WordListSet
{
public:
    std::vector<WordList> m_data;
    std::size_t size() const
    {
        return m_data.size();
    }

    void getUnique(WordListSet &out)
    {
        std::set<std::size_t> uniqueHashTable;

        std::hash<WordList> hasher;
        for (WordList &item : m_data)
        {
            auto hashcode = hasher(item);
            if (uniqueHashTable.find(hashcode) == uniqueHashTable.end())
            {
                out.m_data.push_back(item);
                uniqueHashTable.insert(hashcode);
            }
        }
    }
};

class NodeDictionary
{
public:
    ~NodeDictionary()
    {
        m_lookup.clear();
        for (Node *node : m_nodes)
        {
            delete node;
        }
        m_nodes.clear();
    }
    void build(std::vector<std::string> uniqueWords)
    {
        for (const std::string &word : uniqueWords)
        {
            const uint32_t hashcode = wordHash(word);
            auto pos = m_lookup.find(hashcode);
            if (pos == m_lookup.end())
            {
                Node *node = new Node(hashcode, word);
                m_lookup.insert(std::make_pair(hashcode, node));
                m_nodes.push_back(node);
            }
            else
            {
                pos->second->addWord(word);
            }
        }
    }
    int generateEdges()
    {
        int connections{0};
        auto nodeCount = size();

        for (uint32_t i = 0; i < nodeCount - 1; ++i)
        {
            Node &first = *m_nodes[i];
            for (uint32_t j = i + 1; j < nodeCount; ++j)
            {
                Node &second = *m_nodes[j];
                if (!second.hasOverlap(first))
                {
                    first.connect(second);
                    connections++;
                }
            }
        }
        return connections;
    }
    std::size_t size() const
    {
        return m_nodes.size();
    }
    std::map<uint32_t, Node *> m_lookup{};
    std::vector<Node *> m_nodes{};

    Node &operator[](uint32_t index)
    {
        return *(m_nodes[index]);
    }
    Node &findFromWordHash(uint32_t hash)
    {
        return *(m_lookup[hash]);
    }

    void findAllCliques(WordListSet &out)
    {
        std::cout << "Processing, this will take 1-2 minutes\n";
        auto nodeCount = size();
        for (uint32_t i = 0; i < nodeCount; ++i)
        {
            Node *base = m_nodes[i];
            base->find_cliques(out.m_data);
            std::cout << "Iter " << i << "/" << nodeCount << ". Found " << out.size() << "\r" << std::flush;
        }
        std::cout << "\n";
    }

    void findAllCliquesMultiThread(WordListSet &finalOut, uint32_t numThreads)
    {
        if (m_nodes.size() < numThreads)
        {
            // no point having more threads than nodes
            numThreads = static_cast<uint32_t>(m_nodes.size());
        }
        if (numThreads <= 1)
        {
            return findAllCliques(finalOut);
        }

        std::vector<std::unique_ptr<std::thread>> workers{};

        workers.resize(numThreads);

        std::mutex finalOutMtx{};

        std::queue<Node *> processQueue{};
        std::mutex processQueueMtx{};

        uint32_t processed{0u};
        std::mutex processedMutex{};

        std::condition_variable onConsumed{};

        for (Node *node : m_nodes)
        {
            // fill the queue
            processQueue.push(node);
        }

        const auto nodeCount = processQueue.size();

        auto workerThread = [&]()
        {
            std::vector<WordList> result;
            while (true)
            {
                Node *proc = nullptr;
                {
                    std::lock_guard<std::mutex> myLock(processQueueMtx);
                    if (processQueue.empty())
                    {
                        // no more to process
                        break;
                    }
                    proc = processQueue.front();
                    processQueue.pop();
                }
                proc->find_cliques(result);
                {
                    std::lock_guard<std::mutex> myLock(processedMutex);
                    processed++;
                    onConsumed.notify_one();
                }
            }
            {
                // append result
                std::lock_guard<std::mutex> myLock(finalOutMtx);
                for (const WordList &item : result)
                {
                    finalOut.m_data.push_back(item);
                }
            }
        };

        for (uint32_t i = 0; i < numThreads; ++i)
        {
            workers[i].reset(new std::thread(workerThread));
        }

        // Log the progress while we wait
        while (processed < nodeCount)
        {
            int i;
            {
                std::unique_lock<std::mutex> myLock(processedMutex);
                onConsumed.wait(myLock);
                i = processed;
            }
            std::cout << "Iter " << i << "/" << nodeCount << "\r" << std::flush;
        }
        // All the workers should halt once the queue is empty
        for (uint32_t i = 0; i < numThreads; ++i)
        {
            workers[i]->join();
        }
        workers.clear();
        std::cout << "\nFound " << finalOut.size() << std::endl;
    }
};

int main(int argc, char **argv)
{
    int threads = 1;

    for (int iArg = 1; iArg < argc; iArg++)
    {
        char *arg = argv[iArg];
        if (strlen(arg) >= 3 && arg[0] == '-' && arg[1] == 't')
        {
            arg += 2;
            threads = std::stoi(std::string(arg));
            std::cout << "Using " << threads << " threads\n";
        }
    }

    Timer timer{};
    timer.tick();

    // Load all the words
    std::vector<std::string> allWords;
    loadWordList(INPUT_FILE, allWords, CLIQUE_DEPTH);
    std::cout << "Loaded " << allWords.size() << " " << CLIQUE_DEPTH << " letter words from " << INPUT_FILE
              << std::endl;

    // Find words with all unique characters
    std::vector<std::string> uniqueWords;
    findUniqueWords(allWords, uniqueWords);
    std::cout << "Found " << uniqueWords.size() << " words with unique letters" << std::endl;

    // Build nodes based on words
    NodeDictionary allNodes{};
    allNodes.build(uniqueWords);
    const uint32_t nodeCount = static_cast<uint32_t>(allNodes.size());
    std::cout << "Found " << nodeCount << " unique letter combinations" << std::endl;

    // Connect nodes together if they don't share any letters
    int connections = allNodes.generateEdges();
    std::cout << "Generated " << connections << " node edges" << std::endl;

    // Find cliques
    WordListSet allFoundWordLists;
    allFoundWordLists.m_data.reserve(nodeCount);
    allNodes.findAllCliquesMultiThread(allFoundWordLists, threads);

    // Remove duplicates
    WordListSet uniqueWordLists;
    allFoundWordLists.getUnique(uniqueWordLists);
    std::cout << "Found " << uniqueWordLists.size() << " unique results\n";

    // Expand each set of nodes and create multiple entries combinatorially
    std::vector<std::string> lines;

    std::function<void(const WordList &, int, StringList &)> expand =
        [&lines, &allNodes, &expand](const WordList &words, int index, StringList &accumulate)
    {
        uint32_t word = words[index];
        const Node &node = allNodes.findFromWordHash(word);
        for (std::string word : node.getWords())
        {
            accumulate[index] = word;
            if ((index + 1) == WordList::capacity)
            {
                lines.push_back(join(accumulate));
            }
            else
            {
                expand(words, index + 1, accumulate);
            }
        }
    };

    std::cout << "Expanding " << uniqueWordLists.size() << " results to find all combinations\n";
    for (const WordList item : uniqueWordLists.m_data)
    {
        StringList stringsTemp;
        expand(item, 0, stringsTemp);
    }
    std::cout << "Found " << lines.size() << " combinations\n";

    // Alphabetical order output
    std::sort(lines.begin(), lines.end());

    // Save to csv
    std::fstream fs(OUTPUT_FILE, std::ios::out);
    if (fs.is_open())
    {
        std::cout << "Writing to " << OUTPUT_FILE << "\n";
        for (std::string line : lines)
        {
            fs << line << "\n";
        }
        fs.close();
    }
    else
    {
        std::cout << "Failed to open output file " << OUTPUT_FILE << " for writing :(\n";
    }

    auto algoTook = timer.tock();

    std::cout << "Total time: " << std::setprecision(5) << (algoTook / 1000.0) << " seconds" << std::endl;

    return 0;
}
