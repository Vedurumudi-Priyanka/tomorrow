#include "function.hpp"

static constexpr inline bool isDigit(char c) noexcept
{
    if ('0' <= c && c <= '9')
    {
        return true;
    }
    else
    {
        return false;
    }
};


static inline bool namesVar(char c) noexcept
{
    if ('a' <= c && c <= 'z')
    {
        return true;
    }
    else
    {
        return false;
    }
};


static inline bool isNext(const std::string &source, unsigned int pos, std::function<bool(char)> isTargetChar)
{
    for (unsigned int i = pos; i < source.length(); i++)
    {
        if (source[i] != source[pos])
        {
            if (isTargetChar(source[i]))
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }
    // std::cout << "[ isNext(source: \'" << source << "\', pos: \'" << pos << "\') ]: Entire rest of string from pos only contains the start char.\n";

    return false;
};

static inline bool isNext(const std::string &source, unsigned int pos, std::function<bool(char)> isSimilar, std::function<bool(char)> isTargetChar)
{
    for (unsigned int i = pos; i < source.length(); i++)
    {
        if (!isSimilar(source[i]))
        {
            if (isTargetChar(source[i]))
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }
    // std::cout << "[ isNext(source: \'" << source << "\', pos: \'" << pos << "\') ]: Entire rest of string from pos only contains the start char.\n";

    return false;
};

static inline bool isNext(const std::string &source, unsigned int pos, std::function<bool(char)> isSimilar, std::function<bool(char)> isTargetChar, unsigned int &out_len)
{
    for (unsigned int i = pos; i < source.length(); i++)
    {
        if (!isSimilar(source[i]))
        {
            if (isTargetChar(source[i]))
            {
                out_len = i - pos;
                return true;
            }
            else
            {
                return false;
            }
        }
    }
    // std::cout << "[ isNext(source: \'" << source << "\', pos: \'" << pos << "\') ]: Entire rest of string from pos only contains the start char.\n";

    return false;
};

static inline bool isNext(const std::string &source, unsigned int pos, std::function<bool(char)> isTargetChar, unsigned int &out_len)
{
    for (unsigned int i = pos; i < source.length(); i++)
    {
        if (source[i] != source[pos])
        {
            if (isTargetChar(source[i]))
            {
                out_len = i - pos;
                return true;
            }
            else
            {
                return false;
            }
        }
    }
    // std::cout << "[ isNext(source: \'" << source << "\', pos: \'" << pos << "\') ]: Entire rest of string from pos only contains the start char.\n";

    return false;
}

inline bool Function::isVariable(unsigned int pos, const std::string &source, elemContainer::parsedElem latest, elemContainer &element_out) noexcept
{
    if (source[pos] == '-' && isNext(source, pos, namesVar) && !latest.isValueType && source[pos + 2] != '^')
    {
        return true; // e.g. 7*-x but not -x^2
    }
    if (namesVar(source[pos]))
    {
        unsigned int len;
        bool followedByBracket = isNext(source, pos, namesVar, [](char c) { return c == '('; }, len);
        if (Element::operationState::signatureToFuncMap.count(source.substr(pos, len)) && followedByBracket)
        {
            element_out.successfulParse = true;
            element_out.elements.erase(element_out.elements.begin());
            element_out.elements.push_back(std::make_shared<OperatorElement>(source.substr(pos, len)));
            element_out.elemLength = len;
            element_out.latestElem = {true, false};
            return false; // Can be read as a function call, so that takes priority.
        }
        return true;
    }
    else
    {
        return false;
    }
};

const std::map<const char, Function::operatorInfo> Function::operatorSet = {{'+', {true, false}}, {'-', {true, false}}, {'*', {true, false}}, {'/', {true, false}}, {'^', {true, false}}, {'(', {false, true}}};

void Function::printOpState(const Element::operationState *const opState, bool all) const noexcept
{
    std::cout << "------------- BEGIN OPSTATE --------------\n";
    std::cout << "[opState]:\n";
    if (all)
    {
        std::cout << "[opLevels]: \n";
        std::cout << "{";
        for (auto v : opState->opLevels)
        {
            std::cout << "{ ";
            for (std::string c : v)
            {
                std::cout << "'" << c << "',";
           }
            std::cout << "},\n";
        }

        std::cout << "[varValues]: \n";
        std::cout << "{";
        for (std::pair<char, float> v : opState->variableValues)
        {
            std::cout << "{ '" << v.first << "': " << v.second << "},\n";
        }
    }
    if (opState->data[0])
    {
        std::cout << "[data]: \n";
        for (auto v : opState->data)
        {
            if (v)
            {
                std::cout << v->toString() << ",\n";
            }
            else
            {
                std::cout << "[FAULT]: VOID POINTER-TO-ELEMENT IN OPSTATE->DATA\n";
            }
        }
    }
    else
    {
        std::cout << "[FAULT]: VOID POINTER-TO-ELEMENT AS FIRST IN OPSTATE->DATA\n";
    }

    std::cout << "[scanPos]: {" << opState->scanPosition << "} out of {" << opState->data.size() - 1 << "}\n";
    std::cout << "[executionLevel]: {\n";
    for (auto c : opState->executionLevel)
    {
        std::cout << "{ ";
        std::cout << "'" << c << "',";
        std::cout << "},\n";
    }
    std::cout << "}}\n------------- END OPSTATE ----------------\n";
};

std::string Function::toString() const noexcept
{
    std::stringstream ss;
    ss << "Function{";
    for (auto e : this->_data)
    {
        ss << "e{" << e->toString() << "},";
    }
    ss << "}";
    return ss.str();
};

struct parsedElem
{
public:
    bool isOperatorType;
    bool isValueType;
};

Function::Function(const std::string &source) noexcept
{
    // Go through each char in the source,
    elemContainer::parsedElem latest;
    for (unsigned int i = 0; i < source.length(); i++)
    {
        // Try to parse an element from that position,
        auto elementContainer = getElementAt(i, source, latest);
        if (elementContainer.successfulParse == true)
        {
            // If we successfully found an element, put it into our data buffer.
            for (auto elem : elementContainer.elements)
            {
                this->_data.push_back(elem);
            }
            // this->_data.push_back(elementContainer.elem);
            if (this->debugMode)
            {
                std::cout << "[Function]: data buf looks like: ";
                if (_data[0])
                {
                    std::cout << "[data]: \n";
                    for (auto v : _data)
                    {
                        if (v)
                        {
                            std::cout << v->toString() << ",\n";
                        }
                        else
                        {
                            std::cout << "[FAULT]: VOID POINTER-TO-ELEMENT IN OPSTATE->DATA\n";
                        }
                    }
                }
            }
            /*
            std::cout << "[Function]: data buf looks like: ";
            if(_data[0]){
                std::cout << "[data]: \n";
                for(auto v : _data){
                   if(v){
                      std::cout << v->toString() << ",\n";
                   }
                   else {
                      std::cout << "[FAULT]: VOID POINTER-TO-ELEMENT IN OPSTATE->DATA\n";
                   }
                }
            } 
            */
            latest = elementContainer.latestElem;
            // Move back i so that the next character we look at is just after the one that parsed this elem.
            i += elementContainer.elemLength - 1;
        }
        else
        {
            // Couldn't parse from char.
            std::cout << "[FUNCTION::FUNCTION(" << source << ")]: Couldn't parse from char \'" << source[i] << " at {" << i << "}. Parse error.\n";
        }
    }
    // Post-parse run through.
    for (unsigned int i = 0; i < _data.size() - 1; i++)
    {
        // if(_data[i]->getType() == 'b' && _data[i+1]->getType() == 'b'){
        // this->_data.insert(_data.begin()+i+1, std::make_shared<OperatorElement>('*'));
        // }
        if ((_data[i]->getType() == 'n' || _data[i]->getType() == 'v' || _data[i]->getType() == 'b') && (_data[i + 1]->getType() == 'n' || _data[i + 1]->getType() == 'v' || _data[i + 1]->getType() == 'b' || _data[i+1]->getType() == 'u'))
        {
            this->_data.insert(_data.begin() + i + 1, std::make_shared<OperatorElement>('*'));
        }
        i++;
    }
    if (this->debugMode)
    {
        std::cout << "[Function]: final, post-reprocess data buf looks like: ";
        if (_data[0])
        {
            std::cout << "[data]: \n";
            for (auto v : _data)
            {
                if (v)
                {
                    std::cout << v->toString() << ",\n";
                }
                else
                {
                    std::cout << "[FAULT]: VOID POINTER-TO-ELEMENT IN OPSTATE->DATA\n";
                }
            }
        }
    }
};

float Function::call(std::map<char, float> varMap) const noexcept
{
    Element::operationState opState = {_data, varMap, 0, Element::operationState::opLevels[0]};
    // Go through each operation level (aka go down BIDMAS),
    for (auto operationLevel : Element::operationState::opLevels)
    {
        opState.executionLevel = operationLevel;
        // printOpState(&opState, true);
        // While we're scanning within the bounds of our list of elements,
        while (opState.scanPosition < (opState.data.size() - 1))
        {
            // Call each element, so that the operators can carry out their corresponding actions.
            // printOpState(&opState, true);
            opState.data[opState.scanPosition]->call(&opState, opState.scanPosition);
            opState.scanPosition++;
        }
        // Reset the scanPosition so that the next operationLevel starts at the start.
        opState.scanPosition = 0;
    }
    // std::cout << "END OPSTATE:\n";
    // printOpState(&opState, true);

    // Return the value of the final single element.
    opState.isValueCall = true;
    return opState.data[0]->call(&opState, opState.scanPosition);
};

// Bracket utility functions
static unsigned int matchBracket(const std::string &source, unsigned int pos) noexcept
{
    int nOfSubBrackets = 0;
    for (unsigned int i = pos; i < source.length(); i++)
    {
        if (source[i] == '(')
        {
            nOfSubBrackets++;
        }
        if (source[i] == ')')
        {
            nOfSubBrackets--;
            if (nOfSubBrackets == 0)
            {
                return i;
            }
        }
    }
    std::cout << "[ Function::getElementAt -> matchBracket(source: {" << source << "}, pos: " << pos << ")]: Bracket at that position was not matched by an end-bracket. Parse error.\n";
    return pos;
}
Function::elemContainer Function::getElementAt(unsigned int pos, const std::string &source, elemContainer::parsedElem latest) const noexcept
{
    if (source[pos] == '(')
    {
        auto lenOfBracketExpression = matchBracket(source, pos) - pos;
        auto bracketExpressionFunc = std::make_shared<Function>(source.substr(pos + 1, lenOfBracketExpression - 1));
        return elemContainer(true, {std::make_shared<BracketElement>(bracketExpressionFunc)}, lenOfBracketExpression + 1, {false, true});
    }
    if (isNumeric(source, pos, latest))
    {
        std::string buf;
        bool encounteredNum = false;
        unsigned int nOfNegatives = 0;
        for (unsigned int i = pos; i <= source.length(); i++)
        {
            if (isDigit(source[i]))
            {
                encounteredNum = true;
                buf.push_back(source[i]);
            }
            else if (!encounteredNum && source[i] == '-')
            {
                nOfNegatives++;
                buf.push_back(source[i]);
            }
            else if (source[i] == '.')
            {
                if (!encounteredNum)
                {
                    buf.push_back('0');
                }
                buf.push_back(source[i]);
            }
            else
            {
                break;
            }
        }
        if (nOfNegatives)
        {
            if (nOfNegatives % 2)
            { // In this case, buf.length()
                // Odd number of negatives
                buf.erase(std::remove(buf.begin() + 1, buf.end(), '-'), buf.end());
                return elemContainer(true, {std::make_shared<NumericElement>(std::stof(buf))}, buf.length() + nOfNegatives - 1, {false, true});
            }
            else
            { // In this case, buf.length() + nOfNegatives
                buf.erase(std::remove(buf.begin(), buf.end(), '-'), buf.end());
                return elemContainer(true, {std::make_shared<NumericElement>(std::stof(buf))}, buf.length() + nOfNegatives, {false, true});
            }
        }
        return elemContainer(true, {std::make_shared<NumericElement>(std::stof(buf))}, buf.length(), {false, true});
    }
    // Pass in end pointer.
    else if (auto element = elemContainer(false, {std::make_shared<Element>()}, 1, {false, false});
             isVariable(pos, source, latest, element))
    {
        if (source[pos] == '-')
        {
            unsigned int nOfNegatives = 0;
            isNext(source, pos, namesVar, nOfNegatives);
            if (nOfNegatives % 2)
            {
                return elemContainer(true, {std::make_shared<VariableElement>(source[pos + nOfNegatives], 0)}, 1 + nOfNegatives, {false, true});
            }
            return elemContainer(true, {std::make_shared<VariableElement>(source[pos + nOfNegatives], 1)}, 1 + nOfNegatives, {false, true});
        }
        return elemContainer(true, {std::make_shared<VariableElement>(source[pos], 1)}, 1, {false, true});
    }
    else
    { // HACK: can't get if-init working
        auto elem = getOperator(pos, source, latest);
        if (elem.successfulParse == true)
        {
            return elem;
        }
        else
        {
            return elemContainer(false, {std::make_shared<Element>()}, 1, {false, false});
        }
    }
};

inline Function::elemContainer Function::getOperator(unsigned int pos, const std::string &source, elemContainer::parsedElem latest) noexcept
{
    if (operatorSet.count(source[pos]))
    {
        // If we this character is found in our opSet, and the context suggests it can be an operator
        if (latest.isValueType)
        {
            if (source[pos] == '-' && source[pos + 1] == '-')
            {
                if (source[pos] != '-')
                {
                    std::cout << "[ Function::getOperator(" << pos << ", " << source << ")] Found char in opSet, latest is ValueType and next char is a negative, but we aren't a negative - is this an error? Please report.\n";
                }
                unsigned int nOfNegatives = 2;
                unsigned int index = 0;
                while (source[pos + 2 + index] == '-')
                {
                    nOfNegatives++;
                    index++;
                }
                if (nOfNegatives % 2)
                {
                    return elemContainer(true, {std::make_shared<OperatorElement>('-')}, nOfNegatives, {true, false});
                }
                else
                {
                    return elemContainer(true, {std::make_shared<OperatorElement>('+')}, nOfNegatives, {true, false});
                }
            }
            return elemContainer(true, {std::make_shared<OperatorElement>(source[pos])}, 1, {true, false});
        }
        else if (source[pos] == '-' || source[pos] == '+')
        { // TODO: figure out if the second condition is necesary
            if (pos == 0)
            {
                return elemContainer(true, {std::make_shared<NumericElement>(0.0f), std::make_shared<OperatorElement>(source[pos])}, 1, {true, false});
            }
            return elemContainer(true, {std::make_shared<OperatorElement>(source[pos])}, 1, {true, false});
        }
        return elemContainer(false, {std::make_shared<OperatorElement>('0')}, 1, {false, false});
    }
    // Look for a function call, such as f(...) or sin(...)
    else if (unsigned int len; isNext(source, pos, [](char c) { return namesVar(c); }, [](char c) { return c == '('; }, len))
    {
        if (auto nameStr = Element::operationState::signatureToFuncMap.at(source.substr(pos, len)); nameStr) // If the substring we found is a known function.
        {
            return elemContainer(true, {std::make_shared<OperatorElement>(source.substr(pos, len))}, len, {true, true});
        }
        // Else, probably a multiply like xyz(7)
    }
    // Else,
    return elemContainer(false, {std::make_shared<OperatorElement>('0')}, 1, {false, false});
};

// This function needs to decide if we parse this point into a NumericElement or not.
inline bool Function::isNumeric(const std::string &source, unsigned int start, elemContainer::parsedElem latest) noexcept
{
    if (source[start] == '-')
    {
        if (latest.isOperatorType && !latest.isValueType)
        {                                                // If there's an operator on the left that's not a bracket.
            return isNumeric(source, start + 1, latest); // Return whether or not the character on the right of the minus is a number.
        }
        else
        {
            if (start == 0)
            { // We're at the beginning of the string
                // Find next non- '-'. What is it?
                for (unsigned int i = start; i < source.length(); i++)
                {
                    if (source[i] != '-')
                    {
                        if (isDigit(source[i]))
                        {
                            return true;
                        }
                        else
                        {
                            return false;
                        }
                    }
                }
            }
            return false;
        }
    }
    return isDigit(source[start]);
};

static inline unsigned int findNumberEnd(unsigned int start, const std::string &source) noexcept
{
    unsigned int len = 0;
    for (unsigned int i = start; i < source.length(); i++)
    {
        if (isDigit(source[i]))
        {
            len++;
        }
    }
    // We got to the end of the string.
    return start + len;
};
