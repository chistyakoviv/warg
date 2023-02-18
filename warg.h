#pragma once

#include <iostream>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <memory>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <cstring>
#include <cassert>
#include <sstream>

/**
/   What arguments project is a lib for parsing command line arguments.
*/
namespace warg {

template <typename T>
void convertStringTo(const std::string& str, T& res) {
    std::stringstream ss(str);
    ss >> res;
    if (!ss || ss.fail()) {
        throw std::invalid_argument("couldn't convert string '" + str + "'");
    }
}

template <typename T>
bool isBoolean() {
    return false;
}

template <>
bool isBoolean<bool>() {
    return true;
}

template <typename T>
bool isNullCStr(T&) {
    return false;
}

bool isNullCStr(char* str) {
    return str == nullptr;
}

void throwUnknownParamException(const std::string& pos) {
    throw std::invalid_argument("unknown parameter at: " + pos);
}

class BaseType {
public:
    BaseType(std::string name, std::string description)
        : name(name), description(description)
    {}

    virtual ~BaseType() = default;

    virtual void parse(const std::string&) = 0;
    virtual std::string getValue() = 0;
public:
    std::string name;
    std::string description;
};

template <typename T>
class Type : public BaseType {
public:
    Type() = default;

    Type(std::string name, std::string description, T& type)
        : BaseType(name, description), type(type) {}

    void parse(const std::string& str) override {
        try {
            bool isBool = isBoolean<T>();
            if (isBool && str.empty()) {
                convertStringTo("1", type);
            } else {
                convertStringTo(str, type);
            }
        } catch (const std::invalid_argument& e) {
            std::stringstream ss;
            ss << "'" << str << "' is incorrect input for argument '" << name << "':" << e.what();
            throw std::invalid_argument(ss.str());
        }
    }

    std::string getValue() override {
        std::stringstream ss;
        if (!isNullCStr(type)) {
            ss << type;
        }
        return ss.str();
    }
private:
    T& type;
};

class Argument {
public:
    Argument()
        : matched(false) {}

    template <typename T>
    Argument(std::string name, std::string description, T& targetVar)
        : matched(false),
          target(std::make_unique<Type<T>>(name, description, targetVar)) {}

    Argument(const Argument& other) = delete;

    Argument& operator=(const Argument& other) = delete;

    Argument(Argument&& other) noexcept {
        swap(other);
    }

    Argument& operator=(Argument&& other) noexcept {
        swap(other);
        return *this;
    }

    std::string getName() const {
        return target->name;
    }

    std::string getDescription() const {
        return target->description;
    }

    std::string getValue() const {
        return target->getValue();
    }

    void parse(const std::string& str) {
        target->parse(str);
        matched = true;
    }

    bool isMatched() const {
        return matched;
    }
private:
    void swap(Argument& other) {
        std::swap(matched, other.matched);
        std::swap(target, other.target);
    }
private:
    std::unique_ptr<BaseType> target;
    bool matched;
};

class ArgPack {
public:
    ArgPack() = default;

    template <typename T>
    ArgPack& add(T& target, const std::string& name, const std::string& description) {
        Argument arg(name, description, target);
        registerArg(std::move(arg));
        if (isBoolean<T>()) {
            boolArgNames.insert(name);
        }
        return *this;
    }

    // Positional paremeter is a parameter specified without a key
    template <typename T>
    ArgPack& addPositional(T& target, const std::string& name, const std::string& description) {
        add(target, name, description);
        positionalArgNames.push_back(name);
        return *this;
    }

    void parse(int argc, const char** argv) {
        size_t positionalIndex = 0;
        for (int i = 1; i < argc; ++i) {
            Argument* pArg = nullptr;
            std::string argValue;

            const char* const begin = argv[i];
            const char* const end = begin + std::strlen(argv[i]);
            const char* const assignPos = std::find(begin, end, '=');

            if (assignPos != end) {
                // The string has a assign symbol,
                // try to split the string and interpret the part
                // before the assign symbol as an argument name
                std::string foundArgName(begin, assignPos);
                argMapType::iterator it = args.find(foundArgName);

                if (it != args.end()) {
                    pArg = &it->second;
                    argValue = std::string(assignPos + 1, end);
                } else {
                    throwUnknownParamException(argv[i]);
                }
            } else {
                // Check if the argument is a boolean flag
                argMapType::iterator it = args.find(argv[i]);
                if (it != args.end()) {
                    pArg = &it->second;
                    // Leave the argument value empty
                    // argValue = "";
                } else if (positionalIndex < positionalArgNames.size()) {
                    // Otherwise the string could only be a positional argument.
                    // It's safe to request an argument by a position,
                    // because positional arguments are added to both the dictionary and the positional array
                    const std::string& argName = positionalArgNames[positionalIndex];
                    pArg = &args[argName];
                    argValue = argv[i];
                    positionalIndex++;
                } else {
                    throwUnknownParamException(argv[i]);
                }
            }
            assert(pArg);
            if (pArg->isMatched()) {
                throw std::invalid_argument("several values has been specified for '" + pArg->getName() + "' argument");
            }
            pArg->parse(argValue);
        }
    }

    std::string showHelp(const std::string& binaryName) {
        std::stringstream options;
        std::stringstream description;

        for (const std::string& argName : argsDisplayOrder) {
            const Argument& arg = args[argName];
            bool isBool = boolArgNames.find(argName) != boolArgNames.end();
            options << " [" << arg.getName() << (isBool ? "" : "=value") << "]";
            description << " " << arg.getName() << " - " << arg.getDescription() << "\n";//<< " (" << arg.getValue() << ")\n";
        }
        for (const std::string& positionalArgName : positionalArgNames) {
            options << " [" << positionalArgName;
        }
        for (size_t i = 0; i < positionalArgNames.size(); ++i) {
            options << "]";
        }
        return "Usage: " + binaryName + options.str() + "\n\nOptions:\n" + description.str();
    }
private:
    void registerArg(Argument&& arg) {
        const std::string& argName = arg.getName();
        if (args.find(argName) != args.end()) {
            throw std::invalid_argument("argument with name: '" + argName +
                                        "' already registered");
        }
        args[argName] = std::move(arg);
        argsDisplayOrder.push_back(argName);
    }
private:
    using argMapType = std::unordered_map<std::string, Argument>;
    using argsDisplayOrderType = std::vector<std::string>;
    using boolArgNamesType = std::unordered_set<std::string>;
    using positionalArgNamesType = std::vector<std::string>;

    argMapType args;
    argsDisplayOrderType argsDisplayOrder;
    // Store boolean arguments separately to distinguish them
    // when printing usage information
    boolArgNamesType boolArgNames;
    positionalArgNamesType positionalArgNames;
};

void parse(int argc, char** argv, ArgPack& cliArguments) {
    bool showHelp = false;
    bool invalidUsage = false;
    cliArguments.add(showHelp, "-h", "show usage info");

    try {
        cliArguments.parse(argc, const_cast<const char**>(argv));
    } catch (const std::exception& e) {
        std::cerr << "error parsing command line arguments.\n"
                  << "error message: " << e.what() << "\n";
        invalidUsage = true;
    }
    if (showHelp || invalidUsage) {
        std::cout << cliArguments.showHelp(argv[0]) << "\n";
        std::exit(0);
    }
}

}