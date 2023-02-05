#include <iostream>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <memory>
#include <vector>
#include <set>

/**
/   What arguments project is a lib for parsing command line arguments.
*/
namespace warg {

template <typename T>
void convertTo(const std::string& str, T& res) {
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

class BaseType {
public:
    BaseType(std::string name, std::string description)
        : name(name), description(description)
    {}

    virtual ~BaseType() = default;

    virtual void parse(const std::string str) = 0;
    virtual std::string getValue() = 0;
public:
    std::string name;
    std::string description;
};

template <typename T>
class Type : public BaseType {
public:
    Type(std::string name, std::string description, T& type)
        : BaseType(name, description), type(type)
    {}

    void parse(const std::string& str) override {
        try {
            bool isBool = isBoolean<T>();
            if (isBool && str.empty()) {
                convertTo("1", type);
            } else {
                convertTo(str, type);
            }
        } catch (const std::invalid_argument& e) {
            std::stringstream ss;
            ss << "'" << str << "' is incorrect input for argument '" << name << "':" << e.what();
            throw std::invalid_argument(str.str());
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
    template <typename T>
    Argument(std::string name, std::string description, T& targetVar) : matched(false) {
        target = std::make_unique(name, description, targetVar);
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
    std::unique_ptr<BaseType> target;
    bool matched;
};

class ArgPack {
public:
    ArgPack() = default;

    template <typename T>
    ArgPack& add(T& target, const std::string& name, const std::string& description) {
        Argument arg(name, description, target);
        registerArg(arg);
        if (isBoolean<T>()) {
            boolArgsNames.insert(name);
        }
        return *this;
    }

    void parse(int argc, const char** argv) {

    }

    std::string showHelp() {

    }
private:
    void registerArg(const Argument& arg) {
        auto [_, inserted] = args.insert(std::make_pair(arg.getName(), arg));
        if (!inserted) {
            throw std::invalid_argument("argument with name: '" + arg.getName() +
                                        "' already registered");
        }
        argsDisplayOrder.push_back(arg.getName());
    }
private:
    using argMapType = std::unordered_map<std::string, Argument>;
    using argsDisplayOrderType = std::vector<std::string>;
    using boolArgsNamesType = std::set<std::string>;

    argMapType args;
    argsDisplayOrderType argsDisplayOrder;
    boolArgsNamesType boolArgsNames;
};

void parse(int argc, char** argv, ArgPack& cliArguments) {
    bool showHelp = false;
    bool invalidUsage = false;
    cliArguments.add(showHelp, "-h", "show usage info");

    try {
        cliArguments.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "error parsing command line arguments.\n"
                  << "error message: " << e.what() << "\n";
        invalidUsage = true;
    }
    if (showHelp || invalidUsage) {
        std::cout << cliArguments.showHelp() << "\n";
        std::exit(0);
    }
}

}