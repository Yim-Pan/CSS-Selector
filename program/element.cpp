#ifndef ELEMENT_CPP
#define ELEMENT_CPP

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <sstream>

enum class NodeType
{
    Element = 1,
    Text = 3,
};

struct Node
{
    NodeType nodeType;
    virtual ~Node() {}
    virtual void print(int depth = 0) const = 0;
};

struct Element : public Node, public std::enable_shared_from_this<Element>
{
    std::string tagName;
    std::vector<std::pair<std::string, std::string>> attributes;
    std::weak_ptr<Element> parent;
    std::vector<std::shared_ptr<Node>> children;

private:
    void printAttributes(std::ostream &out) const
    {
        for (const auto &[key, value] : attributes)
        {
            out << ' ' << key << "=\"" << value << '"';
        }
    }

    void printChildren(std::ostream &out, int depth) const
    {
        for (const auto &child : children)
        {
            child->print(depth + 1);
        }
    }

    std::string getIndent(int depth) const
    {
        return std::string(depth * 2, ' ');
    }

public:
    Element()
    {
        nodeType = NodeType::Element;
    }

    void print(int depth = 0) const override
    {
        const auto indent = getIndent(depth);
        std::cout << indent << '<' << tagName;
        printAttributes(std::cout);
        std::cout << '>' << std::endl;
        printChildren(std::cout, depth);
        std::cout << indent << "</" << tagName << '>' << std::endl;
    }

    std::string toString() const
    {
        std::ostringstream oss;
        print(0);
        return oss.str();
    }
};
std::shared_ptr<Element> createElement(const std::string &tagName)
{
    auto elem = std::make_shared<Element>();
    elem->tagName = tagName;
    elem->nodeType = NodeType::Element;
    return elem;
}

struct Text : public Node
{
    std::string nodeValue;
    Text() { nodeType = NodeType::Text; }
    void print(int depth = 0) const override
    {
        std::cout << std::string(depth * 2, ' ') << nodeValue << std::endl;
    }
};

template <typename StringType>
std::shared_ptr<Text> createTextNode(StringType &&content)
{
    auto node = std::make_shared<Text>();
    node->nodeValue = std::forward<StringType>(content);
    node->nodeType = NodeType::Text;
    return node;
}

std::shared_ptr<Text> createTextElement(const std::string &data)
{
    return createTextNode(data);
}
#endif