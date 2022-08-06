
#include <algorithm>
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <ostream>
#include <set>
#include <sstream>
#include <stdint.h>
#include <string>
#include <vector>

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

bool isWordCharsUnique(const std::string &word)
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
    bool operator==(const FixedVector &other) const
    {
        for (uint32_t i = 0u; i < _capacity; ++i)
        {
            if (data[i] != other[i])
            {
                return false;
            }
        }
        return true;
    }
    bool operator!=(const FixedVector &other) const
    {
        return !((*this) == other);
    }
    Type data[_capacity];
};

template <typename Type, uint32_t _capacity>
struct std::hash<FixedVector<Type, _capacity>>
{
    using _dtype = FixedVector<Type, _capacity>;
    std::size_t operator()(_dtype const &items) const noexcept
    {
        std::size_t seed{0u};
        std::hash<Type> hasher;
        for (const Type &item : items)
        {
            // Hash the number itself and then xor the seeds, as we don't care about word order
            seed ^= hasher(item);
        }
        return seed;
    }
};

using WordList = FixedVector<uint32_t, CLIQUE_DEPTH>;
using StringList = FixedVector<std::string, CLIQUE_DEPTH>;

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
        m_edges.push_back(&other);
        other.m_edges.push_back(this);
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
            const Node &first = *m_edges[i1];
            // Don't need to check first hash as all edges already guarantee no clash with this node
            // if((first.m_hash & hash0) != 0) continue;
            const uint32_t hash1 = first.m_hash | hash0;

            for (i2 = i1 + 1; i2 < count; i2++)
            {
                const Node &second = *m_edges[i2];
                if ((second.m_hash & hash1) != 0)
                    continue;
                const uint32_t hash2 = second.m_hash | hash1;

                for (i3 = i2 + 1; i3 < count; i3++)
                {
                    const Node &third = *m_edges[i3];
                    if ((third.m_hash & hash2) != 0)
                        continue;
                    const uint32_t hash3 = third.m_hash | hash2;

                    for (i4 = i3 + 1; i4 < count; i4++)
                    {
                        const Node &fourth = *m_edges[i4];
                        if ((fourth.m_hash & hash3) != 0)
                            continue;
                        WordList output;
                        output[0] = fourth.m_hash;
                        output[1] = third.m_hash;
                        output[2] = second.m_hash;
                        output[3] = first.m_hash;
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
        for (uint32_t i = i0; i < count; ++i)
        {
            const Node &node = *m_edges[i];
            // Node has overlap
            if ((node.m_hash & hash) != 0)
                continue;
            // Store the new node
            current[index] = node.m_hash;
            // If we're the last word to be added
            if (index == 0)
            {
                // Add to result vec
                out.push_back(current);
            }
            else
            {
                // recurse to find the next node
                find_next(out, current, i + 1, count, node.m_hash | hash, index - 1);
            }
        }
    }

    const uint32_t m_hash;
    std::vector<Node *> m_edges{};
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
        if (isWordCharsUnique(word))
        {
            out.push_back(word);
        }
    }
}

class WordSets
{
public:
    std::vector<WordList> m_data;
    std::size_t size() const
    {
        return m_data.size();
    }

    void getUnique(WordSets &out)
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

    void findAllCliques(WordSets &out)
    {
        auto nodeCount = size();
        for (uint32_t i = 0; i < nodeCount; ++i)
        {
            Node *base = m_nodes[i];
            base->find_cliques(out.m_data);
            std::cout << "Iter " << i << "/" << nodeCount << ". Found " << out.size() << "\r" << std::flush;
            // if (out.size() > 0)
            //     break;
        }
    }
};

int main(int argc, char **argv)
{
    Timer timer{};

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
    WordSets wordSet;
    wordSet.m_data.reserve(nodeCount);
    timer.tick();
    allNodes.findAllCliques(wordSet);
    auto algoTook = timer.tock();
    std::cout << "\nTook " << algoTook << "ms\n";

    // Remove duplicates
    WordSets unique;
    wordSet.getUnique(unique);
    std::cout << "Found " << unique.size() << " unique results\n";

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

    std::cout << "Expanding " << unique.size() << " results to find all combinations\n";
    for (const WordList item : unique.m_data)
    {
        StringList stringsTemp;
        expand(item, 0, stringsTemp);
    }
    std::cout << "Found " << lines.size() << " combinations\n";

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

    return 0;
}
