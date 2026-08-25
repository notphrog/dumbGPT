#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <cstdlib>
#include <ctime>
#include <cctype>
#include <thread>
#include <chrono>
#include <random>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

void makeFolder(const std::string& path) {
#ifdef _WIN32
    _mkdir(path.c_str());
#else
    mkdir(path.c_str(), 0777);
#endif
}

// turns all letters to lowercase so stuff like "YeS" works fine
std::string toLowerStr(const std::string& input) {
    std::string result = input;
    for (char& c : result) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

// checks if the user typed just one word with no spaces
bool isSingleWord(const std::string& str) {
    if (str.empty()) return false;
    for (char c : str) {
        if (std::isspace(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

// custom hash function so std::unordered_map can handle std::vector<std::string> keys
struct VectorStringHash {
    std::size_t operator()(const std::vector<std::string>& vec) const {
        std::size_t seed = vec.size();
        for (const auto& s : vec) {
            seed ^= std::hash<std::string>{}(s) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

class TextEngine {
private:
    int MAX_N = 5;
    // std::map<std::vector<std::string>, std::vector<std::string>> nGramMap; using vectorstring hash now
    std::unordered_map<std::vector<std::string>, std::vector<std::string>, VectorStringHash> nGramMap;

    // splits the raw text into words and punctuation
    std::vector<std::string> tokenize(const std::string& rawText) {
        std::vector<std::string> tokens;
        std::string currentWord = "";

        for (size_t i = 0; i < rawText.length(); ++i) {
            char ch = rawText[i];

            // treat single quotes as part of a word if they're inside letters (e.g. don't, it's)
            if (ch == '\'' && !currentWord.empty() && i + 1 < rawText.length() && std::isalpha(static_cast<unsigned char>(rawText[i + 1]))) {
                currentWord += ch;
            }
            else if (std::ispunct(static_cast<unsigned char>(ch))) {
                if (!currentWord.empty()) {
                    tokens.push_back(currentWord);
                    currentWord = "";
                }
                std::string punctToken(1, ch);
                tokens.push_back(punctToken);
            } 
            else if (std::isspace(static_cast<unsigned char>(ch))) {
                if (!currentWord.empty()) {
                    tokens.push_back(toLowerStr(currentWord));
                    currentWord = "";
                }
            } 
            else {
                currentWord += ch;
            }
        }
        if (!currentWord.empty()) {
            tokens.push_back(toLowerStr(currentWord));
        }
        return tokens;
    }

    // teaches the ai which words usually come after other words
    void trainOnTokens(const std::vector<std::string>& tokens) {
        if (tokens.empty()) return;

        for (size_t i = 0; i < tokens.size(); ++i) {
            for (int ctxLen = 1; ctxLen < MAX_N; ++ctxLen) {
                if (i >= static_cast<size_t>(ctxLen)) {
                    std::vector<std::string> context(tokens.begin() + (i - ctxLen), tokens.begin() + i);
                    std::string nextToken = tokens[i];
                    nGramMap[context].push_back(nextToken);
                }
            }
        }
    }

public:
    void setMaxN(int n) {
        MAX_N = n;
    }

    // clears everything out of the brain
    void clearBrain() {
        nGramMap.clear();
    }

    // reads a txt file and trains the ai on it
    bool trainFromFile(const std::string& absolutePath) {
        if (absolutePath.length() < 4 || absolutePath.substr(absolutePath.length() - 4) != ".txt") {
            return false;
        }

        std::ifstream file(absolutePath);
        if (!file.is_open()) {
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        std::vector<std::string> tokens = tokenize(buffer.str());
        trainOnTokens(tokens);
        return true;
    }

    // checks if a token is just punctuation
    bool isPunctuation(const std::string& tok) const {
        return (tok.length() == 1 && std::ispunct(static_cast<unsigned char>(tok[0])));
    }

    // checks if we hit the end of a sentence
    bool isSentenceEnd(const std::string& tok) const {
        return (tok == "." || tok == "!" || tok == "?");
    }

    std::string normalizeToken(std::string token) {
        for (char &c : token) {
            c = std::tolower(static_cast<unsigned char>(c));
        }
        return token;
    }

    // picks the next word and prints out the thinking process in real time
    std::string predictNextVisual(const std::vector<std::string>& history) {
        int maxLookback = std::min(static_cast<int>(history.size()), MAX_N - 1);

        for (int len = maxLookback; len >= 1; --len) {
            // FIX: Ensure history actually has enough tokens before subtracting iterators
            if (history.size() < static_cast<size_t>(len)) continue;

            std::vector<std::string> ctx(history.end() - len, history.end());
            for (auto& word : ctx) {
                word = toLowerStr(word);
            }
            auto it = nGramMap.find(ctx);
            if (it != nGramMap.end() && !it->second.empty()) {
                const auto& choices = it->second;

                std::map<std::string, int> freqs;
                for (const auto& c : choices) freqs[c]++;

                std::cout << "  [context depth: " << (len + 1) << "-gram]\n";
                std::cout << "  [candidates pool]: ";
                for (const auto& pair : freqs) {
                    double pct = (double)pair.second / choices.size() * 100.0;
                    std::cout << "\"" << pair.first << "\" (" << (int)pct << "%) ";
                }
                std::cout << "\n";

                // fixed with mt19937
                static std::mt19937 rng(std::random_device{}());
                std::uniform_int_distribution<size_t> dist(0, choices.size() - 1);
                std::string chosen = choices[dist(rng)];

                std::cout << "  [selected token]: " << chosen << "\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(150));
                return chosen;
            } else {
                std::cout << "  [no match at " << (len + 1) << "-gram -> backing off...]\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(80));
            }
        }
        return "";
    }

    // generates new text based on the prompt
    std::string generate(const std::string& promptText, int desiredWords, bool stopAtPeriod) {
        std::vector<std::string> promptTokens = tokenize(promptText);
        if (promptTokens.empty()) return "bro give me an actual prompt";

        std::vector<std::string> history = promptTokens;
        std::vector<std::string> outputTokens = promptTokens;

        int wordsGenerated = 0;
        bool lastWasSentenceEnd = isSentenceEnd(promptTokens.back());

        std::cout << "\n[visualizer activated - real-time token decision process]\n\n";

        while (true) {
            size_t maxAllowedWords = desiredWords + 75;

            // check if we reached our target word count
            if (wordsGenerated >= desiredWords) {
                if (!stopAtPeriod) {
                    break; 
                }
                if ((outputTokens.size() > promptTokens.size() && isSentenceEnd(outputTokens.back())) || 
                    wordsGenerated >= maxAllowedWords) { // Safety trigger
                    break;
                }
            }

            std::cout << "step " << (wordsGenerated + 1) << ":\n";
            std::string nextTok = predictNextVisual(history);

            if (nextTok.empty()) {
                std::cout << "  [reached end of known word chains]\n";
                break;
            }

            if (lastWasSentenceEnd && isSentenceEnd(nextTok)) {
                // push to history so the context keeps going but skip adding to visible output
                history.push_back(nextTok);
                std::cout << "  [skipped duplicate sentence end]\n\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            } else {
                std::string formattedTok = nextTok;
                if (lastWasSentenceEnd && !isPunctuation(formattedTok)) {
                    if (!formattedTok.empty()) {
                        formattedTok[0] = std::toupper(static_cast<unsigned char>(formattedTok[0]));
                    }
                    lastWasSentenceEnd = false;
                }

                if (isSentenceEnd(nextTok)) {
                    lastWasSentenceEnd = true;
                }

                outputTokens.push_back(formattedTok);
                history.push_back(nextTok);

                if (!isPunctuation(nextTok)) {
                    wordsGenerated++;
                }
                std::cout << "\n";
            }
        }

        std::string finalOutput = "";
        for (size_t i = 0; i < outputTokens.size(); ++i) {
            std::string tok = outputTokens[i];

            if (i == 0) {
                finalOutput += tok;
            } else if (isPunctuation(tok)) {
                finalOutput += tok;
            } else {
                finalOutput += " " + tok;
            }
        }

        return finalOutput;
    }
};

// gets all saved templates from the registry file
std::vector<std::string> getTemplateIndex() {
    std::vector<std::string> tmps;
    std::ifstream idx("template_registry.txt");
    if (!idx.is_open()) return tmps;
    std::string line;
    while (std::getline(idx, line)) {
        if (!line.empty()) tmps.push_back(line);
    }
    idx.close();
    return tmps;
}

// saves a new template to the registry file
void registerTemplate(const std::string& name) {
    std::ofstream idx("template_registry.txt", std::ios::app);
    if (idx.is_open()) {
        idx << name << "\n";
        idx.close();
    }
}

// checks if the user entered a valid positive number
bool isValidPositiveInt(const std::string& str, int& value) {
    if (!isSingleWord(str)) return false;
    for (char c : str) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    }
    try {
        value = std::stoi(str);
        return value > 0;
    } catch (...) {
        return false;
    }
}

int main() {
    // seeds random so the output isnt the exact same every time
    std::srand(static_cast<unsigned int>(std::time(0)));

    bool keepRunningApp = true;

    while (keepRunningApp) {
        TextEngine ai;
        std::vector<std::string> loadedPaths;

        int userN = 5;
        while (true) {
            std::string nInput = "";
            std::cout << "enter max n-gram size (i would reccomend 2 - 10, but its your pc and your choice): ";
            std::getline(std::cin, nInput);

            if (isValidPositiveInt(nInput, userN) && userN >= 2) {
                ai.setMaxN(userN);
                break;
            }
            std::cout << "invalid size please enter a number >= 2\n\n";
        }


        std::cout << "dynamic " << userN << "-gram ai setup\n\n";

        std::vector<std::string> existingTemplates = getTemplateIndex();
        std::string modeChoice = "";

        // asks if user wants to import files or load a template
        while (true) {
            std::cout << "type 'new' to import files or 'template' to load a template: ";
            std::getline(std::cin, modeChoice);
            modeChoice = toLowerStr(modeChoice);

            if (!isSingleWord(modeChoice)) {
                std::cout << "invalid answer input must be a single word\n\n";
                continue;
            }

            if (modeChoice == "template") {
                if (existingTemplates.empty()) {
                    std::cout << "no templates, please say new\n\n";
                } else {
                    break;
                }
            } else if (modeChoice == "new") {
                break;
            } else {
                std::cout << "invalid answer type 'new' or 'template'\n\n";
            }
        }

        // loads a template if they chose that option
        if (modeChoice == "template") {
            std::cout << "\navailable templates:\n";
            for (const auto& t : existingTemplates) {
                std::cout << "  - " << t << "\n";
            }

            std::string selectedTemplate = "";
            while (true) {
                std::cout << "enter template name: ";
                std::getline(std::cin, selectedTemplate);
                selectedTemplate = toLowerStr(selectedTemplate);

                if (!isSingleWord(selectedTemplate)) {
                    std::cout << "invalid template name, must be one word\n";
                    continue;
                }

                bool found = false;
                for (const auto& t : existingTemplates) {
                    if (toLowerStr(t) == selectedTemplate) {
                        found = true;
                        selectedTemplate = t;
                        break;
                    }
                }

                if (found) {
                    makeFolder("templates");
                    std::ifstream tFile("templates/" + selectedTemplate);
                    if (tFile.is_open()) {
                        std::string pathLine;
                        while (std::getline(tFile, pathLine)) {
                            // remove carriage returns (\r) and whitespace
                            while (!pathLine.empty() && (pathLine.back() == '\r' || pathLine.back() == ' ' || pathLine.back() == '\t')) {
                                pathLine.pop_back();
                            }
                            if (!pathLine.empty()) {
                                if (ai.trainFromFile(pathLine)) {
                                    loadedPaths.push_back(pathLine);
                                }
                            }
                        }
                        tFile.close();
                        std::cout << "successfully loaded template data\n\n";
                        break;
                    } else {
                        std::cout << "invalid file (check file name, path, or type)\n";
                    }
                } else {
                    std::cout << "invalid template name, choose from the list above\n";
                }
            }
        }
        // lets user manually type in file paths to train on
        else if (modeChoice == "new") {
            std::cout << "\ninput data in absolute filepath format. when youre done, say 'done'\n\n";

            while (true) {
                std::string pathInput;
                std::cout << "path: ";
                std::getline(std::cin, pathInput);

                if (toLowerStr(pathInput) == "done") {
                    if (loadedPaths.empty()) {
                        std::cout << "you need to add at least one valid file before saying done\n";
                        continue;
                    }
                    break;
                }

                if (ai.trainFromFile(pathInput)) {
                    std::cout << "yum that was some delicious data! can i have more?\n";
                    loadedPaths.push_back(pathInput);
                } else {
                    std::cout << "invalid file (check file name, path, or type)\n";
                }
            }

            // asks if user wants to save those paths as a template
            std::string saveAns = "";
            while (true) {
                std::cout << "\nno more food? :C well wanna save these files as a template? (yes/no): ";
                std::getline(std::cin, saveAns);
                saveAns = toLowerStr(saveAns);

                if (!isSingleWord(saveAns)) {
                    std::cout << "invalid answer say 'yes' or 'no'\n";
                    continue;
                }

                if (saveAns == "yes") {
                    std::string tName = "";
                    while (true) {
                        std::cout << "enter template name (must end in .txt): ";
                        std::getline(std::cin, tName);
                        tName = toLowerStr(tName);

                        if (!isSingleWord(tName)) {
                            std::cout << "invalid file (check file name, path, or type)\n";
                            continue;
                        }

                        if (tName.length() >= 4 && tName.substr(tName.length() - 4) == ".txt") {
                            makeFolder("templates");
                            std::ofstream outFile("templates/" + tName);
                            if (outFile.is_open()) {
                                for (const auto& p : loadedPaths) {
                                    outFile << p << "\n";
                                }
                                outFile.close();
                                registerTemplate(tName);
                                std::cout << "thanks for all the food! template saved as " << tName << "\n\n";
                                break;
                            } else {
                                std::cout << "invalid file (check file name, path, or type)\n";
                            }
                        } else {
                            std::cout << "invalid file (check file name, path, or type)\n";
                        }
                    }
                    break;
                } else if (saveAns == "no") {
                    break;
                } else {
                    std::cout << "invalid answer say 'yes' or 'no'\n";
                }
            }
        }

        std::cout << "ai training finished, delicious data! thanks! C:\n\n";

        bool keepSessionAlive = true;

        // main loop for chatting and generating text
        while (keepSessionAlive) {
            std::string prompt = "";
            while (true) {
                std::cout << "enter prompt (or type 'exit' to quit): ";
                std::getline(std::cin, prompt);
                if (!prompt.empty()) break;
                std::cout << "prompt cannot be empty\n";
            }

            if (toLowerStr(prompt) == "exit") {
                break;
            }

            int wordCount = 0;
            while (true) {
                std::string countInput = "";
                std::cout << "how many words to generate: ";
                std::getline(std::cin, countInput);

                if (isValidPositiveInt(countInput, wordCount)) {
                    break;
                }
                std::cout << "invalid number, try again\n";
            }

            // ask about stopping at nearest period
            bool stopAtPeriod = false;
            while (true) {
                std::string periodChoice = "";
                std::cout << "finish at nearest period? (nearest/no): ";
                std::getline(std::cin, periodChoice);
                periodChoice = toLowerStr(periodChoice);

                if (!isSingleWord(periodChoice)) {
                    std::cout << "invalid answer say 'nearest' or 'no'\n";
                    continue;
                }

                if (periodChoice == "nearest") {
                    stopAtPeriod = true;
                    break;
                } else if (periodChoice == "no") {
                    stopAtPeriod = false;
                    break;
                } else {
                    std::cout << "invalid answer say 'nearest' or 'no'\n";
                }
            }

            bool keepRefreshing = true;
            while (keepRefreshing) {
                std::string response = ai.generate(prompt, wordCount, stopAtPeriod);

                std::cout << "\ngenerated output:\n";
                std::cout << response << "\n\n";

                // saves output text to a txt file if wanted
                while (true) {
                    std::string saveOutputAns = "";
                    std::cout << "wanna save this generated text to a txt file? (yes/no): ";
                    std::getline(std::cin, saveOutputAns);
                    saveOutputAns = toLowerStr(saveOutputAns);

                    if (!isSingleWord(saveOutputAns)) {
                        std::cout << "invalid answer say 'yes' or 'no'\n";
                        continue;
                    }

                    if (saveOutputAns == "yes") {
                        while (true) {
                            std::string outputPath = "";
                            std::cout << "enter absolute file path to save (must end in .txt): ";
                            std::getline(std::cin, outputPath);

                            if (outputPath.length() >= 4 && toLowerStr(outputPath.substr(outputPath.length() - 4)) == ".txt") {
                                std::ofstream saveFile(outputPath);
                                if (saveFile.is_open()) {
                                    saveFile << response << "\n";
                                    saveFile.close();
                                    std::cout << "successfully saved output to " << outputPath << "\n\n";
                                    break;
                                } else {
                                    std::cout << "invalid file (check file name, path, or type)\n";
                                }
                            } else {
                                std::cout << "invalid file (check file name, path, or type)\n";
                            }
                        }
                        break;
                    } else if (saveOutputAns == "no") {
                        break;
                    } else {
                        std::cout << "invalid answer say 'yes' or 'no'\n";
                    }
                }

                // asks if user wants to generate again with same settings
                while (true) {
                    std::string regenChoice = "";
                    std::cout << "wanna regenerate with the same settings? (yes/no): ";
                    std::getline(std::cin, regenChoice);
                    regenChoice = toLowerStr(regenChoice);

                    if (!isSingleWord(regenChoice)) {
                        std::cout << "invalid answer say 'yes' or 'no'\n";
                        continue;
                    }

                    if (regenChoice == "yes") {
                        keepRefreshing = true;
                        break;
                    } else if (regenChoice == "no") {
                        keepRefreshing = false;
                        break;
                    } else {
                        std::cout << "invalid answer say 'yes' or 'no'\n";
                    }
                }
            }
        }

        // asks if user wants to reboot the app completely
        while (true) {
            std::string restartChoice = "";
            std::cout << "\nwanna run the whole program again from scratch? (yes/no): ";
            std::getline(std::cin, restartChoice);
            restartChoice = toLowerStr(restartChoice);

            if (!isSingleWord(restartChoice)) {
                std::cout << "invalid answer say 'yes' or 'no'\n";
                continue;
            }

            if (restartChoice == "yes") {
                keepRunningApp = true;
                std::cout << "\nclearing settings...\n\n";
                break;
            } else if (restartChoice == "no") {
                keepRunningApp = false;
                std::cout << "cya twin\n";
                break;
            } else {
                std::cout << "invalid answer twin say 'yes' or 'no'\n";
            }
        }
    }

    return 0;
}