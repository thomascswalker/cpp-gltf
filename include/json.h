#ifndef JSON_H
#define JSON_H

#define DEBUG_TYPE false

#define IS_NUMBER(x) (((x - 48) | (57 - x)) >= 0 || x == 46 || x == 45)
#define IS_QUOTE(x) x == 34
#define IS_NOT_QUOTE(x) x != 34
#define IS_COMMA(x) x == 44
#define IS_LBRACE(x) x == 91
#define IS_RBRACE(x) x == 93
#define IS_LBRACKET(x) x == 123
#define IS_RBRACKET(x) x == 125
#define IS_COLON(x) x == 58

#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <iterator>
#include <cstddef>

namespace JSON {
    static int CURRENT_INDENT = 0;

    // Forward declaration
    class JsonObject;

    typedef std::vector<JsonObject> JsonArray;
    typedef std::map<std::string, JsonObject> JsonDict;

    struct Token;

    class Lexer;

    class Parser;

    std::string getIndent(int indent);

    std::string formatLine(const std::string &value, int indent, bool end);

    std::string formatLine(const std::string &key, const std::string &value,
                           int indent, bool end);

    JsonObject loadFile(const std::string &filename);

    JsonObject loadString(std::string &string);

    std::ostream &operator<<(std::ostream &o, JsonArray &a);

    std::ostream &operator<<(std::ostream &o, JsonDict &d);

    /// <summary>
    /// JSON value types, with numbers slightly modified for C++.
    /// Number becomes both Int and Double.
    /// </summary>
    enum EValueType {
        Null, // nullptr
        LBrace,
        RBrace,
        LBracket,
        RBracket,
        Colon,
        Comma,
        Number,
        Bool,      // true, false
        Int,       // 1, 2, 3
        Double,    // 3.14, 7.62, 50.50
        String,    // "This is a string."
        Array,     // { 1, 2, 3, 4, 5 }
        Dictionary // { {"Key 1", 5}, {"Key 2", 10} }
    };

    /// <summary>
    /// Base class for all JSON value types.
    /// </summary>
    class Value {
    public:
        /// <summary>
        /// Format the current value to a string.
        /// </summary>
        /// <returns>The string-formatted value.</returns>
        virtual std::string format() = 0;
    };

    /// <summary>
    /// Typedef alias for Value.
    /// </summary>
    typedef Value value_t;

    /// <summary>
    /// NullType JSON value. Represents the 'null' keyword.
    /// </summary>
    class [[maybe_unused]] NullValue : public value_t {
    public:
        NullValue() = default;

        std::string format() override;

        std::ostream &operator<<(std::ostream &o);
    };

    /// <summary>
    /// Boolean JSON value. Represents the 'true' and 'false' keywords.
    /// </summary>
    class BoolValue : public value_t {
        bool m_value{};

    public:
        explicit BoolValue(bool value) : m_value(value) {
        };

        BoolValue(const BoolValue &other);

        [[nodiscard]] bool value() const;

        std::string format() override;

        std::ostream &operator<<(std::ostream &o);
    };

    /// <summary>
    /// Number JSON value. Represents numbers which have no decimals (integers).
    /// </summary>
    class IntValue : public value_t {
        int m_value{};

    public:
        explicit IntValue(int value) : m_value(value) {
        };

        IntValue(const IntValue &other);

        [[nodiscard]] int value() const;

        std::string format() override;

        std::ostream &operator<<(std::ostream &o);
    };

    /// <summary>
    /// Number JSON value. Represents numbers which have decimals.
    /// </summary>
    class DoubleValue : public value_t {
        double m_value{};

    public:
        explicit DoubleValue(double value) : m_value(value) {
        };

        DoubleValue(const DoubleValue &other);

        [[nodiscard]] double value() const;

        std::string format() override;

        std::ostream &operator<<(std::ostream &o);
    };

    /// <summary>
    /// StringType JSON value. Does not contain wrapped quotes, they are not
    /// necessary because of the defined type.
    /// </summary>
    class StringValue : public value_t {
        std::string m_value;

    public:
        explicit StringValue(std::string &value) : m_value(value) {
        };

        explicit StringValue(const std::basic_string<char> &value) : m_value(value) {
        };

        StringValue(StringValue const &other);

        std::string value();

        std::string format() override;

        std::ostream &operator<<(std::ostream &o);
    };

    /// <summary>
    /// Array JSON value. Contains a single array of [JsonObject, ...].
    /// This is defined with the typedef JsonArray.
    /// </summary>
    class ArrayValue : public value_t {
        JsonArray m_value;

    public:
        explicit ArrayValue(const JsonArray &value);

        ArrayValue(const ArrayValue &other);

        JsonArray value();

        JsonArray *ptr();

        std::string format() override;

        ArrayValue &operator=([[maybe_unused]] const ArrayValue &other);

        JsonObject &operator[](int index);

        std::ostream &operator<<(std::ostream &o);

        friend std::ostream &operator<<(std::ostream &o, ArrayValue &a);
    };

    /// <summary>
    /// Array JSON value. Contains a single map of {{std::string, JsonObject}, ...}.
    /// This is defined with the typedef JsonDict.
    /// </summary>
    class DictValue : public value_t {
        JsonDict m_value;

    public:
        explicit DictValue(const JsonDict &value);

        DictValue(const DictValue &other);

        JsonDict value();

        JsonDict *ptr();

        std::string format() override;

        DictValue &operator=(const DictValue &other);

        JsonObject &operator[](const std::string &key);

        friend std::ostream &operator<<(std::ostream &o, DictValue &d);
    };

    /// <summary>
    /// Base JSON object. Contains a wrapper for each possible value type, with
    /// constructors and accessors for each.
    /// </summary>
    class JsonObject {
        std::unique_ptr<value_t> m_value;
        EValueType m_type;

        // https://www.internalpointers.com/post/writing-custom-iterators-modern-cpp
        struct Iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = JsonObject;
            using pointer = JsonObject *;
            using reference = JsonObject &;

        private:
            pointer m_ptr;

        public:
            explicit Iterator(pointer ptr)
                    : m_ptr(ptr) {};

            reference operator*() const { return *m_ptr; }

            pointer operator->() { return m_ptr; }

            // Prefix increment
            Iterator &operator++() {
                m_ptr++;
                return *this;
            }

            // Postfix increment
            Iterator operator++(int) {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            friend bool operator==(const Iterator &a, const Iterator &b) { return a.m_ptr == b.m_ptr; };

            friend bool operator!=(const Iterator &a, const Iterator &b) { return a.m_ptr != b.m_ptr; };
        };

    public:
        // Constructors
        JsonObject();                                  // Default
        JsonObject(const JsonObject &other);           // Copy
        explicit JsonObject(bool value);               // Bool
        explicit JsonObject(int value);                // Integer
        explicit JsonObject(double value);             // Double
        explicit JsonObject(const std::string &value); // StringType
        explicit JsonObject(const JsonArray &value);   // Array
        explicit JsonObject(const JsonDict &value);    // Dictionary

        /// <summary>
        /// Returns the EValueType of this JsonObject.
        /// </summary>
        EValueType type() {
            return m_type;
        }

        [[nodiscard]] BoolValue &asBool() const;

        [[nodiscard]] IntValue &asInt() const;

        [[nodiscard]] DoubleValue &asDouble() const;

        [[nodiscard]] StringValue &asString() const;

        [[nodiscard]] ArrayValue &asArray() const;

        [[nodiscard]] DictValue &asDict() const;

        [[nodiscard]] bool getBool() const;

        [[nodiscard]] int getInt() const;

        [[nodiscard]] double getDouble() const;

        [[nodiscard]] std::string getString() const;

        [[nodiscard]] JsonArray getArray() const;

        [[nodiscard]] JsonDict getDict() const;

        /// <summary>
        /// Formats this JsonObject as a std::string.
        /// </summary>
        [[nodiscard]] std::string format() const;

        bool hasKey(const std::string &key);

        size_t size();

        Iterator begin() {
            if (m_type == Array) {
                return Iterator(&asArray()[0]);
            } else {
                throw std::runtime_error("Invalid type for range.");
            }
        };

        // Operators
        JsonObject &operator=(const JsonObject &other);

        JsonObject &operator[](const std::string &key);

        JsonObject &operator[](int index);

        friend std::ostream &operator<<(std::ostream &o, JsonObject &j);

        friend std::ostream &operator<<(std::ostream &o, const JsonObject &j);

        // Type conversion
        explicit operator bool() const;

        explicit operator int() const;

        explicit operator double() const;

        explicit operator std::string() const;

        explicit operator JsonArray() const;

        explicit operator JsonDict() const;
    };

    /// <summary>
    /// Token struct for lexing.
    /// </summary>
    struct Token {
        EValueType type = EValueType::Null;
        std::string value;
    };

    /// <summary>
    /// Lexer for tokenizing the given input string. By default, the input string
    /// will be sanitized (removing whitespace, new lines, returns, end of file,
    /// etc.)
    /// </summary>
    class Lexer {
        std::string m_string;
        int m_offset = 0;

        /// <summary>
        /// Sanitizes the given input string. This will remove new lines, end of
        /// lines, and whitespace outside of strings.
        /// </summary>
        /// <param name="input">The input string to sanitize.</param>
        /// <returns>The sanitized string.</returns>
        static std::string sanitize(std::string &input);

    public:
        std::vector<Token> tokens;

        explicit Lexer(std::string string);;

        /// <summary>
        /// Determines if we can continue tokenization if the current character
        /// position is not at the end of the sanitized string.
        /// </summary>
        bool canContinue();

        /// <summary>
        /// Determines the next token and adds it to the array of tokens. This will
        /// increment `m_offset` by however long the token is determined to be.
        /// </summary>
        /// <returns>The token which is constructed.</returns>
        Token next();
    };

    /// <summary>
    /// Parser which ingests a Lexer (essentially a list of tokens) and builds an
    /// Abstract Syntax Tree (AST) from it. The final output of this AST is a
    /// JsonObject itself.
    /// </summary>
    class Parser {
        // The lexer which contains the tokens to parse.
        Lexer *m_lexer;

        // The output JsonObject.
        JsonObject m_json;

        // The current token pointer.
        Token *current;

        // The current token position.
        int pos = 0;

        /// <summary>
        /// Iterate to the next token as well as bump the position by 1.
        /// </summary>
        void next();

//#pragma clang diagnostic push
//#pragma ide diagnostic ignored "misc-no-recursion"

        /// <summary>
        /// Recursively parse the current token. Assuming the JSON string is valid and
        /// begins with either a dictionary or an array, the first token will begin
        /// the recursion.
        /// </summary>
        /// <returns></returns>
        JsonObject parse();

//#pragma clang diagnostic pop

    public:
        explicit Parser(Lexer *lexer);

        /// <summary>
        /// Returns the JsonObject which was parsed from the file (or string).
        /// </summary>
        JsonObject &get();
    };
} // namespace JSON

#endif
