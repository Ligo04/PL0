#include "PLO_ProgramParser.h"


namespace PL0
{
    bool ProgramParser::Parse(_In_ const std::vector<Symbol>& symbols)
    {
        m_ProgramInfo.identifiers.clear();
        m_ProgramInfo.instructions.clear();
        m_TempSelfSum.clear();

        m_pCurrSymbol = symbols.cbegin();
        m_pEndSymbol = symbols.cend();

        m_ErrorInfos.clear();

        // ***程序的入口点为_main_***
        m_ProgramInfo.identifiers.push_back({"_main_",ID_PROCEDURE,0,0,0,});
        m_ProgramInfo.instructions.push_back({ Func_JMP,0,0 });

        ParseProgram();


        return m_ErrorInfos.empty();
    }

    const std::vector<ErrorInfo>& ProgramParser::GetErrorInfos() const
    {
        return m_ErrorInfos;
    }

    const ProgramInfo& ProgramParser::GetProgramInfo() const
    {
        return m_ProgramInfo;
    }

    bool ProgramParser::ParseProgram()
    {
        // <程序>          ::= <常量说明部分><变量说明部分><过程说明部分><语句>.

        std::vector<Identifier> constants;
        std::vector<Identifier> variables;
        bool isOK = ParseConstDesc(constants);
        isOK = isOK && ParseVarDesc(variables);

        // **交换常量与变量位置，确定初始栈大小
        int initStackUnit = 0;
        for (auto& id : variables)
        {
            // **索引偏移，第一个变量的地址从0开始
            if (!(m_ProgramInfo.identifiers.back().kind & ID_PROCEDURE))
            {
                id.offset = 1 + m_ProgramInfo.identifiers.back().offset;
            }
            ++initStackUnit;
            m_ProgramInfo.identifiers.push_back(id);
        }
        
        for (auto& id : constants)
        {
            m_ProgramInfo.identifiers.push_back(id);
        }

        isOK = isOK && ParseProcedureDesc();

        // ***回填程序地址***
        m_ProgramInfo.identifiers.front().offset = static_cast<int>(m_ProgramInfo.instructions.size());
        m_ProgramInfo.instructions.front().mix = m_ProgramInfo.identifiers.front().offset;

        // ***开辟栈空间***
        m_ProgramInfo.instructions.push_back({ Func_INT,0,initStackUnit });

        isOK = isOK && ParseStatement();

        // ***主过程结束***
        m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_RET });

        if (!isOK)
            return false;

        // .
        if (m_pCurrSymbol == m_pEndSymbol || m_pCurrSymbol->symbolType != ST_Period)
        {
            try
            {
                SafeCheck(ST_Error, 39);		// [Error039] '.' Expected.
            }
            catch (const std::exception&)
            {
                return false;
            }
        }

        ++m_pCurrSymbol;

        if (m_pCurrSymbol == m_pEndSymbol)
        {
            return true;
        }
        else
        {
            try
            {
                SafeCheck(ST_Error, 41);		// [Error041] Unexpected content after '.'.
            }
            catch (const std::exception&)
            {
                return false;
            }
            return false;
        }
    }

    bool ProgramParser::ParseConstDesc(_Inout_ std::vector<Identifier>& constants)
    {
        try
        {
            // <常量说明部分>  ::= {const <值类型><常量定义>{,<常量定义>};}

            //const
            while (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_Const)
            {
                ++m_pCurrSymbol;

                SymbolType valueType = ST_Null;
                // <值类型>
                if (m_pCurrSymbol == m_pEndSymbol || m_pCurrSymbol->symbolType != ST_Int)
                {
                    SafeCheck(ST_Error, 31);                //[Error031] Typename expected
                }
                valueType = m_pCurrSymbol->symbolType;
                ++m_pCurrSymbol;

                // {,<常量定义>};

                ParseConstDef(valueType, constants);


                // {,<常量定义>};

                while (m_pCurrSymbol!=m_pEndSymbol&&m_pCurrSymbol->symbolType==ST_Comma)
                {
                    // ,
                    ++m_pCurrSymbol;
                    // <常量定义>
                    ParseConstDef(valueType, constants);
                }

                // ;
                SafeCheck(ST_SemiColon, 21);              // [Error021] ';' expeted
                ++m_pCurrSymbol;
            }
        }
        catch (std::exception)
        {
            if (m_pCurrSymbol != m_pEndSymbol)
            {
                //跳过当前语句
                while (m_pCurrSymbol != m_pEndSymbol && (m_pCurrSymbol->symbolType != ST_SemiColon))
                {
                    ++m_pCurrSymbol;
                }
            }
            return false;
        }
        return true;
    }

    void ProgramParser::ParseConstDef(_In_ SymbolType type, _Inout_ std::vector<Identifier>& constants)
    {
        // <常量定义> ::= <id>:=<值>


        // <id>
        SafeCheck(ST_Identifier, 22);                   // [Error022] Identifier Expected
        // ***id重定义检测***
        int level = 0;
        if (~GetIDIndex(m_pCurrSymbol->word, &level) && level == m_CurrLevel)
        {
            SafeCheck(ST_Error, 34);                    // [Error034] Identifier redefined
        }

        //*** 添加常量id***
        Identifier id = { "",ID_CONST,0,m_CurrLevel,0 };
        strcpy_s(id.name, m_pEndSymbol->word.c_str());
        switch (type)
        {
        case ST_Int: id.kind |= ID_INT;
            break;
        }
        ++m_pCurrSymbol;

        // :=
        SafeCheck(ST_Assign, 23);                      // [Error023] ':=' Expected
        ++m_pCurrSymbol;

        // <值>
        if (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_Integer && type == ST_Int)
        {
            id.value = std::stoi(m_pCurrSymbol->word);
            constants.push_back(id);
            ++m_pCurrSymbol;
        }
        else
        {
            SafeCheck(ST_Error, 32);                    // [Error032] Value expected
        }
    }

    bool ProgramParser::ParseVarDesc(_Inout_ std::vector<Identifier>& variables)
    {
       
        // <变量说明部分>  ::= {<值类型><变量定义>{,<变量定义>};}
        try
        {
            while (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_Int)
            {
                SymbolType valueType{};

                // <值类型>
                valueType = m_pCurrSymbol->symbolType;
                ++m_pCurrSymbol;

                // <变量定义>
                ParseVarDef(valueType, variables);


                // {,<变量定义>};}

                // ,
                while (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_Comma)
                {
                    ++m_pCurrSymbol;

                    ParseVarDef(valueType, variables);
                }

                // ;
                SafeCheck(ST_SemiColon, 21);	// [Error021] ';' expected.
                ++m_pCurrSymbol;
            }
        }
        catch (std::exception)
        {
            if (m_pCurrSymbol != m_pEndSymbol)
            {
                //跳过当前语句
                while (m_pCurrSymbol != m_pEndSymbol && (m_pCurrSymbol->symbolType != ST_SemiColon))
                {
                    ++m_pCurrSymbol;
                }
            }
            return false;
        }
        return true;
    }

    void ProgramParser::ParseVarDef(_In_ SymbolType type, _Inout_ std::vector<Identifier>& variables)
    {
        //<变量定义>      ::= <id>[:=<值>]

         // <id>
        SafeCheck(ST_Identifier, 22);                   // [Error022] Identifier Expected
        // ***id重定义检测***
        int level = 0;
        if (~GetIDIndex(m_pCurrSymbol->word, &level) && level == m_CurrLevel)
        {
            SafeCheck(ST_Error, 34);                    // [Error034] Identifier redefined
        }

        //添加变量id
        Identifier id{ "",ID_VAR,0,m_CurrLevel,0 };
        strcpy_s(id.name, m_pCurrSymbol->word.c_str());
        switch (type)
        {
        case ST_Int:id.kind |= ID_INT;
            break;
        }

        ++m_pCurrSymbol;

        // [:=<值>]
        if (m_pCurrSymbol == m_pEndSymbol || m_pCurrSymbol->symbolType != ST_Assign)
        {
            variables.push_back(id);
            return;
        }

        // ::=
        ++m_pCurrSymbol;

        // <值>
        if (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_Integer && type == ST_Int)
        {
            id.value = std::stoi(m_pCurrSymbol->word);
            variables.push_back(id);
            ++m_pCurrSymbol;
        }
        else
        {
            SafeCheck(ST_Error, 32);	// [Error032] Value expected.
        }
    }

    bool ProgramParser::ParseProcedureDesc()
    {
        try
        {
            // <过程说明部分>  ::= <过程首部><分程序>;{<过程说明部分>}

            while (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_Procedure)
            {
                // <过程首部> 
                ParseProcedureHeader();


                // <分程序>
                ParseSubProcedure();


                // ;
                SafeCheck(ST_SemiColon, 21);	// [Error021] ';' expected.
                ++m_pCurrSymbol;

                // {<过程说明部分>}
                while (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_Procedure)
                {
                    // <过程说明部分>
                    ParseProcedureDesc();
                }
            }
        } 
        catch (std::exception)
        {
            if (m_pCurrSymbol != m_pEndSymbol)
            {
                // 跳过当前语句
                while (m_pCurrSymbol != m_pEndSymbol &&
                    (m_pCurrSymbol->symbolType == ST_SemiColon))
                {
                    ++m_pCurrSymbol;
                }
            }
            m_CurrLevel = 0;
            return false;
        }
        m_CurrLevel = 0;
        return true;
    }

    void ProgramParser::ParseProcedureHeader()
    {
        // <过程首部>      ::= procedure <id>'('[<值类型><id>{,<值类型><id>}]')';

        // prodecure
        ++m_pCurrSymbol;

        // <id>
        SafeCheck(ST_Identifier, 22);                   // [Error022] Identifier Expected
        // **id重定义检测**
        int level = 0;
        if (~GetIDIndex(m_pCurrSymbol->word, &level))
        {
            SafeCheck(ST_Error, 34);                    // [Error034] Identifier redefined
        }

        // **添加过程id**
        Identifier id = { "",ID_PROCEDURE,0,m_CurrLevel,
            static_cast<int>(m_ProgramInfo.instructions.size()) };
        strcpy_s(id.name, m_pCurrSymbol->word.c_str());
        m_CurrProcIndex = m_ProgramInfo.identifiers.size();
        m_ProgramInfo.identifiers.push_back(id);
        ++m_pCurrSymbol;

        // '('
        SafeCheck(ST_LeftParen, 24);                    // [Error024] '(' Expected.
        ++m_pCurrSymbol;

        // ***从过程的形参开始为内层***
        ++m_CurrLevel;

        // [<值类型><id>{,<值类型><id>}]]')';
        if (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_Int)
        {
            // <值类型>
            SymbolType valueType = m_pCurrSymbol->symbolType;
            ++m_pCurrSymbol;

            // <id>
            SafeCheck(ST_Identifier, 22);                   // [Error022] Identifier Expected
            if (~GetIDIndex(m_pCurrSymbol->word, &level) && level == m_CurrLevel)
            {
                SafeCheck(ST_Error, 34);	// [Error034] Identifier redefined.
            }

            // ***添加过程符号***
            id = { "",ID_PARAMETER | ID_VAR,0,m_CurrLevel,0 };
            strcpy_s(id.name, m_pCurrSymbol->word.c_str());
            switch (valueType)
            {
            case ST_Int:id.kind |= ID_INT;
                break;
            }


            m_ProgramInfo.identifiers.push_back(id);
            ++m_pCurrSymbol;


            // {,<值类型><id>}

            while (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_Comma)
            {
                ++m_pCurrSymbol;
                // <值类型>
                if (m_pCurrSymbol == m_pEndSymbol || m_pCurrSymbol->symbolType != ST_Int)
                {
                    SafeCheck(ST_Error, 31);	// [Error031] Typename expected.
                }
                SymbolType valueType = m_pCurrSymbol->symbolType;
                ++m_pCurrSymbol;

                // <id>
                SafeCheck(ST_Identifier, 22);                   // [Error022] Identifier Expected
                if (~GetIDIndex(m_pCurrSymbol->word, &level) && level == m_CurrLevel)
                {
                    SafeCheck(ST_Error, 34);                    // [Error034] Identifier redefined
                }

                // ***添加过程参数符号***
                id = { "",ID_PARAMETER | ID_VAR,0,m_CurrLevel,0 };
                strcpy_s(id.name, m_pCurrSymbol->word.c_str());
                switch (valueType)
                {
                case ST_Int:id.kind |= ID_INT;
                    break;
                }

                // ***形参索引偏移***
                id.offset = 1 + m_ProgramInfo.identifiers.back().offset;
                m_ProgramInfo.identifiers.push_back(id);
                ++m_pCurrSymbol;
            }
        }

        // ')'
        SafeCheck(ST_RightParen, 25);               // [Error025] ')' Expected.
        ++m_pCurrSymbol;

        // ;
        SafeCheck(ST_SemiColon, 21);                // [Error021] ';' expected.
        ++m_pCurrSymbol;
    }

    void ProgramParser::ParseSubProcedure()
    {
        // <分程序>        ::= <常量说明部分><变量说明部分><语句>

        std::vector<Identifier> constants;
        std::vector<Identifier> variables;
        // <常量说明部分>
        ParseConstDesc(constants);
        // <变量说明部分>
        ParseVarDesc(variables);

        // **交换常量与变量位置，确定初始栈大小
        int initStackUnit = 0;
        for (auto& id : variables)
        {
            // **索引偏移，第一个变量的地址从0开始
            if (!(m_ProgramInfo.identifiers.back().kind & ID_PROCEDURE))
            {
                id.offset = 1 + m_ProgramInfo.identifiers.back().offset;
            }
            ++initStackUnit;
            m_ProgramInfo.identifiers.push_back(id);
        }

        // **获取形参数目
        int paramCount = 0;
        for (uint32_t i = m_ProgramInfo.identifiers.size() - 1; !(m_ProgramInfo.identifiers[i].kind & ID_PROCEDURE); --i)
        {
            if (m_ProgramInfo.identifiers[i].kind & ID_PARAMETER)
                ++paramCount;
        }

        //** 开辟栈空间**
        m_ProgramInfo.instructions.push_back({ Func_INT,0,paramCount });

        for (auto& id : constants)
        {
            m_ProgramInfo.identifiers.push_back(id);
        }

        // <语句>
        ParseStatement();

        // ***返回，并回收栈空间
        m_ProgramInfo.instructions.push_back({ Func_OPR,0,0 });

        // ***过程结束，返回上一层
        --m_CurrLevel;
    }

    bool ProgramParser::ParseStatement()
    {
        try
        {
            // <语句>          ::= <空语句>|<赋值语句>|<条件语句>|<while循环语句>|<for循环语句>|<过程调用语句>|<读语句>|<写语句>|<返回语句>|<复合语句>

            // <空语句>
            if (m_pCurrSymbol == m_pEndSymbol)
                return true;
            // <复合语句>
            else if (m_pCurrSymbol->symbolType == ST_Begin)
            {
                ParseComplexStat();
            }
            // <赋值语句>
            else if (m_pCurrSymbol->symbolType == ST_Identifier || m_pCurrSymbol->symbolType == ST_SelfAdd || m_pCurrSymbol->symbolType == ST_SelfSub)
            {
                ParseAssignStat();
            }
            // <条件语句>
            else if (m_pCurrSymbol->symbolType == ST_If)
            {
                ParseConditionStat();
            }
            // <while循环语句>
            else if (m_pCurrSymbol->symbolType == ST_While)
            {
                ParseWhileLoopStat();
            }
            // <for循环语句>
            else if (m_pCurrSymbol->symbolType == ST_For)
            {
                ParseForLoopStat();
            }
            // <过程调用语句>
            else if (m_pCurrSymbol->symbolType == ST_Call)
            {
                ParseCallStat();
            }
            // <读语句>
            else if (m_pCurrSymbol->symbolType == ST_Read)
            {
                ParseReadStat();
            }
            // <写语句>
            else if (m_pCurrSymbol->symbolType == ST_Write)
            {
                ParseWriteStat();
            }
            // <返回语句>
            else if (m_pCurrSymbol->symbolType == ST_Return)
            {
                ParseReturnStat();
            }
        }
        catch (std::exception)
        {
            if (m_pCurrSymbol != m_pEndSymbol)
            {
                //跳过当前语句
                while (m_pCurrSymbol != m_pEndSymbol &&
                    (m_pCurrSymbol->symbolType != ST_SemiColon))
                {
                    ++m_pCurrSymbol;
                }
            }
            
            return false;
        }
        return true;
    }

    void ProgramParser::ParseAssignStat()
    {
        // <赋值语句>      ::=[++|--] <id> | <id> [++|--] | :=|+=|-=|*=|/=|%= <表达式>


        SymbolType assignMent = ST_Null;
        if (m_pCurrSymbol->symbolType == ST_SelfAdd || m_pCurrSymbol->symbolType == ST_SelfSub)
        {
            assignMent = m_pCurrSymbol->symbolType;
            ++m_pCurrSymbol;
        }
        // <id>
        const std::string& id = m_pCurrSymbol->word;
        int idLevel = 0;
        uint32_t pos = GetIDIndex(id, &idLevel);
        // ***检验id存在性与是否可以写入***
        if (!~pos)
        {
            SafeCheck(ST_Error, 33);                    // [Error033] Unknown identifier.
        }
        else if (!(m_ProgramInfo.identifiers[pos].kind & ID_VAR))
        {
            SafeCheck(ST_Error, 35);	// [Error035] Identifier can't be assigned.
        }
        int idOffet = m_ProgramInfo.identifiers[pos].offset;
        ++m_pCurrSymbol;

        if (assignMent == ST_Null)
        {
            // :=|+=|-=|*=|/=|%=
            if (m_pCurrSymbol == m_pEndSymbol || m_pCurrSymbol->symbolType<ST_Assign || m_pCurrSymbol->symbolType>ST_Equal)
            {
                SafeCheck(ST_Error, 23);	// [Error023] ':=' Expected.
            }

            assignMent = m_pCurrSymbol->symbolType;
            ++m_pCurrSymbol;
        }

        if (assignMent != ST_Assign)
        {
            m_ProgramInfo.instructions.push_back({ Func_LOD,m_CurrLevel - idLevel,idOffet });
        }

        if (assignMent == ST_SelfAdd || assignMent == ST_SelfSub)
        {
            m_ProgramInfo.instructions.push_back({ Func_LIT,0,1 });
        }
        else
        {
            // <表达式>
            ParseExpressionL1();
        }


        // 非常规赋值还需要进行一次二元运算
        switch (assignMent)
        {
        case ST_SelfAdd:
        case ST_PlusAndAssign:m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_ADD });
            break;
        case ST_SelfSub:
        case ST_MinusAndAssign:m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_SUB });
            break;
        case ST_MultiplyAndAssign:m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_MUL });
            break;
        case ST_DivideAndAssign:m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_DIV });
            break;
        case ST_ModAndAssign:m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_MOD });
            break;
        }

        m_ProgramInfo.instructions.push_back({ Func_STO,m_CurrLevel - idLevel,idOffet });


        // ***赋值结束后，若存在后自增/减运算需添加***
        m_ProgramInfo.instructions.insert(m_ProgramInfo.instructions.cend(), m_TempSelfSum.cbegin(), m_TempSelfSum.cend());
    }

    void ProgramParser::ParseComplexStat()
    {
        // <复合语句>      ::= begin <语句>{;<语句>} end
        
        // begin
        ++m_pCurrSymbol;

        // <语句>
        ParseStatement();

        // {;<语句>}
        while (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_SemiColon)
        {
            ++m_pCurrSymbol;

            // <语句>
            ParseStatement();
        }

        // end
        SafeCheck(ST_End, 26);		// [Error026] Keyword 'end' expected.
        ++m_pCurrSymbol;
    }

    void ProgramParser::ParseConditionStat()
    {
        // <条件语句>      ::= if <条件> then <语句> {else if <条件> then <语句>}[else <语句>]

        std::forward_list<int> trueList{};
        std::forward_list<int> falseList{};
        int trueLength = 0, falseLength = 0;
        // if 
        ++m_pCurrSymbol;

        // <条件>
        ParseConditionL1();

        falseList.push_front(m_ProgramInfo.instructions.size());
        m_ProgramInfo.instructions.push_back({ Func_JPC,0,0 });         //地址待回填
        ++falseLength;

        // then
        SafeCheck(ST_Then, 27);		// [Error027] Keyword 'then' expected.
        ++m_pCurrSymbol;

        // 语句
        ParseStatement();

        // {else if <条件> then <语句>}[else <语句>]

        // else
        while (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_Else)
        {
            ++m_pCurrSymbol;

            trueList.push_front(m_ProgramInfo.instructions.size());
            m_ProgramInfo.instructions.push_back({ Func_JMP,0,0 });     //地址待回填
            ++trueLength;

            // if
            if (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_If)
            {
                ++m_pCurrSymbol;

                // <条件>
                ParseConditionL1();

                falseList.push_front(m_ProgramInfo.instructions.size());
                m_ProgramInfo.instructions.push_back({ Func_JPC,0,0 });         //地址待回填
                ++falseLength;

                // then
                SafeCheck(ST_Then, 27);		// [Error027] Keyword 'then' expected.
                ++m_pCurrSymbol;

                //<语句>
                ParseStatement();
            }
            else
            {
                ParseStatement();
                break;
            }
        }

        int currOffset = m_ProgramInfo.instructions.size();

        // ***真/假链地址回填(假链长度可能比真链大1)***
        if (falseLength > trueLength)
        {
            m_ProgramInfo.instructions[falseList.front()].mix = currOffset;
            falseList.pop_front();
        }
        
        while (falseLength-- && trueLength--)
        {
            int trueNodeOffset = trueList.front();
            m_ProgramInfo.instructions[trueList.front()].mix = currOffset;
            m_ProgramInfo.instructions[falseList.front()].mix = trueNodeOffset + 1;

            falseList.pop_front();
            trueList.pop_front();
        }
    }

    void ProgramParser::ParseWhileLoopStat()
    {
        // <while循环语句> ::= while <条件> do <语句>


        ++m_pCurrSymbol;

        int startLoopOffset = m_ProgramInfo.instructions.size();

        // <条件>
        ParseConditionL1();

        // **待回填地址**
        int falseIndex = m_ProgramInfo.instructions.size();
        m_ProgramInfo.instructions.push_back({ Func_JPC,0,0 });


        // do
        SafeCheck(ST_Do, 28);           // [Error028] keyword'do' expected
        ++m_pCurrSymbol;

        ParseStatement();

        m_ProgramInfo.instructions.push_back({ Func_JMP,0,startLoopOffset });

        // ***回填地址***
        m_ProgramInfo.instructions[falseIndex].mix = m_ProgramInfo.instructions.size();
    }

    void ProgramParser::ParseForLoopStat()
    {
        // <for循环语句>   ::= for <id>:=<表达式> step <表达式> until <条件> do <语句>

        ++m_pCurrSymbol;

        // <id>
        SafeCheck(ST_Identifier, 22);               // [Error022] Identifier Expected.
        const std::string& id = m_pCurrSymbol->word;
        int idLevel = 0;
        uint32_t pos = GetIDIndex(m_pCurrSymbol->word, &idLevel);
        if (!(m_ProgramInfo.identifiers[pos].kind & ID_VAR))
        {
            SafeCheck(ST_Error, 35);                // [Error035] Identifier can't be assigned.
        }
        int idOffset = m_ProgramInfo.identifiers[pos].offset;
        ++m_pCurrSymbol;

        // :=
        SafeCheck(ST_Assign, 23);                   // [Error023] ':=' Expected.
        ++m_pCurrSymbol;


        // <表达式>
        ParseExpressionL1();

        m_ProgramInfo.instructions.push_back({ Func_STO,m_CurrLevel - idLevel,idOffset });

        // step
        // ***由于要先判断条件，执行语句块，在进行更新，需要拦截这部分指令***
        SafeCheck(ST_Step, 29);                     // [Error029] Keyword 'step' expected.
        ++m_pCurrSymbol;
        
        uint32_t currInstructCount = m_ProgramInfo.instructions.size();
        std::vector<Instruction> temp{};

        // <表达式>
        ParseExpressionL1();

        // ***临时缓存这些表达式***
        temp.insert(temp.cbegin(), m_ProgramInfo.instructions.cbegin() + currInstructCount, m_ProgramInfo.instructions.cend());
        m_ProgramInfo.instructions.erase(m_ProgramInfo.instructions.cbegin() + currInstructCount);

        // until
        SafeCheck(ST_Until, 30);                    // [Error030] Keyword 'until' expected.
        ++m_pCurrSymbol;

        int startLoopOffset = m_ProgramInfo.instructions.size();

        // <条件>
        ParseConditionL1();

        // **由于条件为假时退出循环，在这里需要逻辑取反
        m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_NOT });
        // **待回填地址
        int falseIndex = m_ProgramInfo.instructions.size();
        m_ProgramInfo.instructions.push_back({ Func_JPC,0,0 });

        // do
        SafeCheck(ST_Do, 28);	// [Error028] Keyword 'do' expected.
        ++m_pCurrSymbol;

        // <语句>
        ParseStatement();

        // **执行step**
        m_ProgramInfo.instructions.push_back({ Func_LOD,m_CurrLevel - idLevel,idOffset });
        m_ProgramInfo.instructions.insert(m_ProgramInfo.instructions.cend(),temp.cbegin(),temp.cend());
        m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_ADD });
        m_ProgramInfo.instructions.push_back({ Func_STO,m_CurrLevel - idLevel,idOffset });
        m_ProgramInfo.instructions.push_back({ Func_JMP,0,startLoopOffset });

        // **回填地址***
        m_ProgramInfo.instructions[falseIndex].mix = m_ProgramInfo.instructions.size();
    }

    void ProgramParser::ParseReadStat()
    {
        // <读语句>        ::= read '('<id>{,<id>}')'

        // read
        ++m_pCurrSymbol;

        // '('
        SafeCheck(ST_LeftParen, 24);	// [Error024] '(' Expected.
        ++m_pCurrSymbol;

        // id
        SafeCheck(ST_Identifier, 22);               // [Error022] Identifier Expected.
        const std::string& id = m_pCurrSymbol->word;
        int idLevel = 0;
        uint32_t pos = GetIDIndex(m_pCurrSymbol->word, &idLevel);
        // **验证id存在性与是否可以写入
        if (!~pos)
        {
            SafeCheck(ST_Error, 33);		// [Error033] Unknown identifier.
        }
        else if (!(m_ProgramInfo.identifiers[pos].kind & ID_VAR))
        {
            SafeCheck(ST_Error, 36);		// [Error036] Identifier is not a procedure.
        }
        int idOffset = m_ProgramInfo.identifiers[pos].offset;
        ++m_pCurrSymbol;

        m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_SCN });
        m_ProgramInfo.instructions.push_back({ Func_STO,m_CurrLevel - idLevel,idOffset });

        // {,<id>}
        while (m_pCurrSymbol!=m_pEndSymbol&&m_pCurrSymbol->symbolType==ST_Comma)
        {
            ++m_pCurrSymbol;

            SafeCheck(ST_Identifier, 22);               // [Error022] Identifier Expected.
            const std::string& id = m_pCurrSymbol->word;
            int idLevel = 0;
            uint32_t pos = GetIDIndex(m_pCurrSymbol->word, &idLevel);
            // **验证id存在性与是否可以写入
            if (!~pos)
            {
                SafeCheck(ST_Error, 33);		// [Error033] Unknown identifier.
            }
            else if (!(m_ProgramInfo.identifiers[pos].kind & ID_VAR))
            {
                SafeCheck(ST_Error, 36);		// [Error036] Identifier is not a procedure.
            }
            int idOffset = m_ProgramInfo.identifiers[pos].offset;
            ++m_pCurrSymbol;
        }


        // ')'
        SafeCheck(ST_RightParen, 25);		// [Error025] ')' Expected.
        ++m_pCurrSymbol;
    }

    void ProgramParser::ParseWriteStat()
    {
        // <写语句>        ::= write '('<表达式>{,<表达式>}')'

        // write
        ++m_pCurrSymbol;

        // '('
        SafeCheck(ST_LeftParen, 24);	// [Error024] '(' Expected.
        ++m_pCurrSymbol;

        // <表达式>
        ParseExpressionL1();

        m_ProgramInfo.instructions.push_back({ Func_OPR, 0, Opr_PRT });

        // {,<表达式>}

        while (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_Comma)
        {
            ++m_pCurrSymbol;

            // <表达式>
            ParseExpressionL1();

            m_ProgramInfo.instructions.push_back({ Func_OPR, 0, Opr_PRT });

        }


        // ')'
        SafeCheck(ST_RightParen, 25);		// [Error025] ')' Expected.
        ++m_pCurrSymbol;
    }

    void ProgramParser::ParseCallStat()
    {
        // <过程调用语句>  ::= call <id>'('[<表达式>{,<表达式>}]')'

        // call
        ++m_pCurrSymbol;
        int paramCount = 0;

        // id
        SafeCheck(ST_Identifier, 22);               // [Error022] Identifier Expected.
        const std::string& id = m_pCurrSymbol->word;
        int idLevel = 0;
        uint32_t pos = GetIDIndex(m_pCurrSymbol->word, &idLevel);
        // **验证id存在性与是否为函数
        if (!~pos)
        {
            SafeCheck(ST_Error, 33);		// [Error033] Unknown identifier.
        }
        else if (!(m_ProgramInfo.identifiers[pos].kind & ID_PROCEDURE))
        {
            SafeCheck(ST_Error, 36);		// [Error036] Identifier is not a procedure.
        }
        int idOffset = m_ProgramInfo.identifiers[pos].offset;
        ++m_pCurrSymbol;

        // '('
        SafeCheck(ST_LeftParen, 24);		// [Error024] '(' Expected.
        ++m_pCurrSymbol;

        // [<表达式>{,<表达式>}]')'

        // <表达式>
        if (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType != ST_RightParen)
        {
            ParseExpressionL1();
            ++paramCount;

            // {,<表达式>}

            // ,
            while (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_Comma)
            {
                ++m_pCurrSymbol;

                // <表达式>
                ParseExpressionL1();
                ++paramCount;
            }
        }

        // ')'
        SafeCheck(ST_RightParen, 25);		// [Error025] ')' Expected.
        ++m_pCurrSymbol;

        // *** 查看函数的实际形参数目***
        int actualParamCount = 0;
        uint32_t endPos = m_ProgramInfo.identifiers.size();
        for (uint32_t i = pos + 1; i < endPos; ++i)
        {
            if (m_ProgramInfo.identifiers[i].kind & ID_PARAMETER)
            {
                ++actualParamCount;
            }
            else
            {
                break;
            }
        }

        if (actualParamCount != paramCount)
        {
            SafeCheck(ST_Error, 37);		// [Error037] Number of function parameter mismatch.
        }


        // ***退避为形参让出空间（仅改变栈顶指针，不移除上面的内容）***
        m_ProgramInfo.instructions.push_back({ Func_POP,0,paramCount });

        // ***最后调用函数***
        m_ProgramInfo.instructions.push_back({ Func_CAL,m_CurrLevel - idLevel,m_ProgramInfo.identifiers[pos].offset });
    }

    void ProgramParser::ParseReturnStat()
    {
        //<返回语句>      ::= return
        ++m_pCurrSymbol;
        m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_RET });
    }

    void ProgramParser::ParseConditionL1()
    {
        // <条件>          ::= <二级条件>{<逻辑或><二级条件>}

        // <二级条件>
        ParseConditionL2();

        // {<逻辑或><二级条件>}
        if (m_pCurrSymbol == m_pEndSymbol || m_pCurrSymbol->symbolType != ST_LogicalOr)
            return;

        std::forward_list<int> trueList{};

        while (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_LogicalOr)
        {
            // 逻辑或
            ++m_pCurrSymbol;

            // ***记录真链***

            m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_NOT });
            trueList.push_front(m_ProgramInfo.instructions.size());
            m_ProgramInfo.instructions.push_back({ Func_JPC,0,0 });

            // <二级条件>
            ParseConditionL2();
        }

        m_ProgramInfo.instructions.push_back({ Func_JMP,0,static_cast<int>(m_ProgramInfo.instructions.size() + 2) });

        while (!trueList.empty())
        {
            m_ProgramInfo.instructions[trueList.front()].mix = m_ProgramInfo.instructions.size();
            trueList.pop_front();
        }

        m_ProgramInfo.instructions.push_back({ Func_LIT,0,1 });
    }

    void ProgramParser::ParseConditionL2()
    {
        // <二级条件>      ::= <三级条件>{<逻辑与><三级条件>}

        // <三级条件>
        ParseConditionL3();

        // {<逻辑与><三级条件>}
        if (m_pCurrSymbol == m_pEndSymbol || m_pCurrSymbol->symbolType != ST_LogicalAnd)
            return;

        std::forward_list<int> falseList{};


        while (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_LogicalAnd)
        {
            // 逻辑与
            ++m_pCurrSymbol;

            // ***记录假链***
            falseList.push_front(m_ProgramInfo.instructions.size());
            m_ProgramInfo.instructions.push_back({ Func_JPC,0,0 });

            // <三级条件>
            ParseConditionL3();
        }

        m_ProgramInfo.instructions.push_back({ Func_JMP,0,static_cast<int>(m_ProgramInfo.instructions.size() + 2) });

        while (!falseList.empty())
        {
            m_ProgramInfo.instructions[falseList.front()].mix = m_ProgramInfo.instructions.size();
            falseList.pop_front();
        }

        m_ProgramInfo.instructions.push_back({ Func_LIT,0,0 });
    }

    void ProgramParser::ParseConditionL3()
    {
        // <三级条件>      ::= <四级条件>{!=|= <四级条件>}
        ParseConditionL4();


        // {!=|= <四级条件>}
        while (m_pCurrSymbol != m_pEndSymbol &&
            m_pCurrSymbol->symbolType == ST_NotEqual || m_pCurrSymbol->symbolType == ST_Equal)
        {
            // !=|=
            SymbolType type = m_pCurrSymbol->symbolType;
            ++m_pCurrSymbol;

            // <四级条件>
            ParseConditionL4();

            switch (type)
            {
            case ST_NotEqual:m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_NEQ });
                break;
            case ST_Equal:m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_EQU });
                break;
            }
        }

    }

    void ProgramParser::ParseConditionL4()
    {
        // <四级条件>      ::= <表达式>{>|>=|<|<= <表达式>}
        ParseExpressionL1();

        // {>|>=|<|<= <表达式>}
        while (m_pCurrSymbol != m_pEndSymbol &&
            m_pCurrSymbol->symbolType == ST_Greater || m_pCurrSymbol->symbolType == ST_GreaterEqual ||
            m_pCurrSymbol->symbolType == ST_Less || m_pCurrSymbol->symbolType == ST_LessEqual)
        {
            // >|>=|<|<=
            SymbolType type = m_pCurrSymbol->symbolType;
            ++m_pCurrSymbol;

            // <表达式>
            ParseExpressionL1();

            switch (type)
            {
            case ST_Greater:m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_GTR });
                break;
            case ST_GreaterEqual:m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_GEQ });
                break;
            case ST_Less:m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_LES });
                break;
            case ST_LessEqual:m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_LEQ });
                break;
            }
        }
    }

    void ProgramParser::ParseExpressionL1()
    {
        // <表达式>        ::= <二级表达式>{+|- <二级表达式>}

        // <二级表达式>
        ParseExpressionL2();

        while (m_pCurrSymbol != m_pEndSymbol &&
            m_pCurrSymbol->symbolType == ST_Plus || m_pCurrSymbol->symbolType == ST_Minus )
        {
            // +|-
            SymbolType type = m_pCurrSymbol->symbolType;
            ++m_pCurrSymbol;

            // <二级表达式>
            ParseExpressionL2();

            switch (type)
            {
            case ST_Plus:m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_ADD });
                break;
            case ST_Minus:m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_SUB });
                break;
            }
        }
    }

    void ProgramParser::ParseExpressionL2()
    {
        // <二级表达式>    ::= <三级表达式>{*|/|% <三级表达式>}

        // <三级表达式>
        ParseExpressionL3();

        while (m_pCurrSymbol!=m_pEndSymbol&&
            m_pCurrSymbol->symbolType==ST_Multiply|| m_pCurrSymbol->symbolType==ST_Divide||m_pCurrSymbol->symbolType==ST_Mod)
        {
            // *|/|%
            SymbolType type = m_pCurrSymbol->symbolType;
            ++m_pCurrSymbol;

            // <三级表达式>
            ParseExpressionL3();
            
            switch (type)
            {
            case ST_Multiply:m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_MUL });
                break;
            case ST_Divide:m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_DIV });
                break;
            case ST_Mod:m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_MOD });
                break;
            }
        }
    }

    void ProgramParser::ParseExpressionL3()
    {
        // <三级表达式>    ::= [!|-|+]<四级表达式>
        
        SymbolType prefix = ST_Null;

        // [!|-|+]
        if (m_pCurrSymbol != m_pEndSymbol)
        {
            if (m_pCurrSymbol->symbolType == ST_LogicalNot ||
                m_pCurrSymbol->symbolType == ST_Minus ||
                m_pCurrSymbol->symbolType == ST_Plus)
            {
                prefix = m_pCurrSymbol->symbolType;
                ++m_pCurrSymbol;
            }
        }

        // <四级表达式>
        ParseExpressionL4();

        switch (prefix)
        {
        case ST_LogicalNot:m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_NOT });
            break;
        case ST_Minus:m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_NEG });
            break;
        }
    }

    void ProgramParser::ParseExpressionL4()
    {

        //  <四级表达式>          ::= [++|--]<id>|<id>[++|--]| |<value>|'('<条件>')'

        if (m_pCurrSymbol == m_pEndSymbol)
        {
            SafeCheck(ST_Error, 38);		// [Error038] Expression/Condition expected.
        }
        else if (m_pCurrSymbol->symbolType == ST_SelfAdd || m_pCurrSymbol->symbolType == ST_SelfSub)
        {
            // [++|--]
            SymbolType assignMent = m_pCurrSymbol->symbolType;
            ++m_pCurrSymbol;

            // <id>
            const std::string& id = m_pCurrSymbol->word;
            int idLevel = 0;
            uint32_t pos = GetIDIndex(id, &idLevel);
            // ***检验id存在性与是否可以写入***
            if (!~pos)
            {
                SafeCheck(ST_Error, 33);                    // [Error033] Unknown identifier.
            }
            else if (!(m_ProgramInfo.identifiers[pos].kind & ID_VAR))
            {
                SafeCheck(ST_Error, 35);	// [Error035] Identifier can't be assigned.
            }
            int idOffet = m_ProgramInfo.identifiers[pos].offset;
            ++m_pCurrSymbol;


            m_ProgramInfo.instructions.push_back({ Func_LOD,m_CurrLevel - idLevel,idOffet });
            m_ProgramInfo.instructions.push_back({ Func_LIT,0,1 });


            switch (assignMent)
            {
            case ST_SelfAdd:m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_ADD });
                break;
            case ST_SelfSub:m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_SUB });
                break;
            }

            m_ProgramInfo.instructions.push_back({ Func_STO,m_CurrLevel - idLevel,idOffet });

            // 取立即数
            m_ProgramInfo.instructions.push_back({ Func_LOD,m_CurrLevel - idLevel,idOffet });

        }
        // <id>
        else if (m_pCurrSymbol->symbolType == ST_Identifier)
        {
            int idLevel = 0;
            uint32_t pos = GetIDIndex(m_pCurrSymbol->word, &idLevel);

            if (pos == UINT_MAX)
            {
                SafeCheck(ST_Error, 33);	// [Error033] Unknown identifier.
            }

            int idOffset = m_ProgramInfo.identifiers[pos].offset;
            // ***常量用指令LIT,变量用指令LOD***
            if (m_ProgramInfo.identifiers[pos].kind & ID_CONST)
            {
                m_ProgramInfo.instructions.push_back({Func_LIT,m_CurrLevel-idLevel,m_ProgramInfo.identifiers[pos].value});
            }
            else
            {
                m_ProgramInfo.instructions.push_back({ Func_LOD,m_CurrLevel - idLevel,idOffset });
            }

            ++m_pCurrSymbol;

            // [++|--]
            if (m_pCurrSymbol->symbolType == ST_SelfAdd || m_pCurrSymbol->symbolType == ST_SelfSub)
            {
                if (!(m_ProgramInfo.identifiers[pos].kind & ID_VAR))
                {
                    SafeCheck(ST_Error,42);         //[Error042] Identifier can't arithmetic.
                }

                //***存放临时自增的指令
                m_TempSelfSum.push_back({ Func_LOD,m_CurrLevel - idLevel,idOffset });
                m_TempSelfSum.push_back({ Func_LIT,0,1 });

                switch (m_pCurrSymbol->symbolType)
                {
                case ST_SelfAdd:m_TempSelfSum.push_back({ Func_OPR,0,Opr_ADD });
                    break;
                case ST_SelfSub:m_TempSelfSum.push_back({ Func_OPR,0,Opr_SUB });
                    break;
                }

                m_TempSelfSum.push_back({ Func_STO,m_CurrLevel - idLevel,idOffset });

                ++m_pCurrSymbol;
            }
        }
        // <value>
        else if (m_pCurrSymbol->symbolType == ST_Integer)
        {
            int value = std::stoi(m_pCurrSymbol->word);
            m_ProgramInfo.instructions.push_back({ Func_LIT,0,value });
            ++m_pCurrSymbol;
        }
        // '('<条件>')'
        else if (m_pCurrSymbol->symbolType == ST_LeftParen)
        {
            ++m_pCurrSymbol;

            // <条件>
            ParseConditionL1();

            // ')'
            SafeCheck(ST_RightParen, 25);		// [Error025] ')' Expected.
            ++m_pCurrSymbol;
        }
        else
        {
            SafeCheck(ST_Error, 40);			// [Error040] Invalid expression.
        }
    }


    void ProgramParser::SafeCheck(SymbolType ST, ErrorCode errorCode, const char* errorStr)
    {
        // 若当前已经扫完整个符号序列，或者当前期望的符号类型与实际的不一致，则记录错误信息并抛出异常
        if (m_pCurrSymbol == m_pEndSymbol || m_pCurrSymbol->symbolType != ST)
        {
            ErrorInfo errorInfo{};
            if (m_pCurrSymbol != m_pEndSymbol)
            {
                errorInfo.beg = m_pCurrSymbol->beg;
                errorInfo.end = m_pCurrSymbol->end;
            }
            else
            {
                --m_pCurrSymbol;
                errorInfo.beg = m_pCurrSymbol->beg;
                errorInfo.end = m_pCurrSymbol->end;
                ++m_pCurrSymbol;
            }
            errorInfo.errorCode = errorCode;
            m_ErrorInfos.push_back(errorInfo);

            throw std::exception(errorStr);
        }
    }

    uint32_t ProgramParser::GetIDIndex(const std::string& str, int* outLevel)
    {
        // 检验当前过程是否有该符号
        uint32_t endIndex = m_ProgramInfo.identifiers.size();
        uint32_t i = 0;
        if (m_CurrLevel > 0)
        {
            for (i = m_CurrProcIndex; i < endIndex; ++i)
            {
                if (m_ProgramInfo.identifiers[i].name == str)
                {
                    if (outLevel)
                        *outLevel = 1;
                    return i;
                }
            }
        }


        // 检验主过程是否有该符号
        i = 1;
        while (i < endIndex && !(m_ProgramInfo.identifiers[i].kind & ID_PROCEDURE))
        {
            if (m_ProgramInfo.identifiers[i].name == str)
            {
                if (outLevel)
                    *outLevel = 0;
                return i;
            }
            ++i;
        }

        // 检验是否为过程
        i = 1;
        while (i < endIndex)
        {
            if (m_ProgramInfo.identifiers[i].name == str && (m_ProgramInfo.identifiers[i].kind & ID_PROCEDURE))
            {
                if (outLevel)
                    *outLevel = 0;
                return i;
            }
            ++i;
        }
        return UINT_MAX;
    }
}
