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

        // ***�������ڵ�Ϊ_main_***
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
        // <����>          ::= <����˵������><����˵������><����˵������><���>.

        std::vector<Identifier> constants;
        std::vector<Identifier> variables;
        bool isOK = ParseConstDesc(constants);
        isOK = isOK && ParseVarDesc(variables);

        // **�������������λ�ã�ȷ����ʼջ��С
        int initStackUnit = 0;
        for (auto& id : variables)
        {
            // **����ƫ�ƣ���һ�������ĵ�ַ��0��ʼ
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

        // ***��������ַ***
        m_ProgramInfo.identifiers.front().offset = static_cast<int>(m_ProgramInfo.instructions.size());
        m_ProgramInfo.instructions.front().mix = m_ProgramInfo.identifiers.front().offset;

        // ***����ջ�ռ�***
        m_ProgramInfo.instructions.push_back({ Func_INT,0,initStackUnit });

        isOK = isOK && ParseStatement();

        // ***�����̽���***
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
            // <����˵������>  ::= {const <ֵ����><��������>{,<��������>};}

            //const
            while (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_Const)
            {
                ++m_pCurrSymbol;

                SymbolType valueType = ST_Null;
                // <ֵ����>
                if (m_pCurrSymbol == m_pEndSymbol || m_pCurrSymbol->symbolType != ST_Int)
                {
                    SafeCheck(ST_Error, 31);                //[Error031] Typename expected
                }
                valueType = m_pCurrSymbol->symbolType;
                ++m_pCurrSymbol;

                // {,<��������>};

                ParseConstDef(valueType, constants);


                // {,<��������>};

                while (m_pCurrSymbol!=m_pEndSymbol&&m_pCurrSymbol->symbolType==ST_Comma)
                {
                    // ,
                    ++m_pCurrSymbol;
                    // <��������>
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
                //������ǰ���
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
        // <��������> ::= <id>:=<ֵ>


        // <id>
        SafeCheck(ST_Identifier, 22);                   // [Error022] Identifier Expected
        // ***id�ض�����***
        int level = 0;
        if (~GetIDIndex(m_pCurrSymbol->word, &level) && level == m_CurrLevel)
        {
            SafeCheck(ST_Error, 34);                    // [Error034] Identifier redefined
        }

        //*** ��ӳ���id***
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

        // <ֵ>
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
       
        // <����˵������>  ::= {<ֵ����><��������>{,<��������>};}
        try
        {
            while (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_Int)
            {
                SymbolType valueType{};

                // <ֵ����>
                valueType = m_pCurrSymbol->symbolType;
                ++m_pCurrSymbol;

                // <��������>
                ParseVarDef(valueType, variables);


                // {,<��������>};}

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
                //������ǰ���
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
        //<��������>      ::= <id>[:=<ֵ>]

         // <id>
        SafeCheck(ST_Identifier, 22);                   // [Error022] Identifier Expected
        // ***id�ض�����***
        int level = 0;
        if (~GetIDIndex(m_pCurrSymbol->word, &level) && level == m_CurrLevel)
        {
            SafeCheck(ST_Error, 34);                    // [Error034] Identifier redefined
        }

        //��ӱ���id
        Identifier id{ "",ID_VAR,0,m_CurrLevel,0 };
        strcpy_s(id.name, m_pCurrSymbol->word.c_str());
        switch (type)
        {
        case ST_Int:id.kind |= ID_INT;
            break;
        }

        ++m_pCurrSymbol;

        // [:=<ֵ>]
        if (m_pCurrSymbol == m_pEndSymbol || m_pCurrSymbol->symbolType != ST_Assign)
        {
            variables.push_back(id);
            return;
        }

        // ::=
        ++m_pCurrSymbol;

        // <ֵ>
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
            // <����˵������>  ::= <�����ײ�><�ֳ���>;{<����˵������>}

            while (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_Procedure)
            {
                // <�����ײ�> 
                ParseProcedureHeader();


                // <�ֳ���>
                ParseSubProcedure();


                // ;
                SafeCheck(ST_SemiColon, 21);	// [Error021] ';' expected.
                ++m_pCurrSymbol;

                // {<����˵������>}
                while (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_Procedure)
                {
                    // <����˵������>
                    ParseProcedureDesc();
                }
            }
        } 
        catch (std::exception)
        {
            if (m_pCurrSymbol != m_pEndSymbol)
            {
                // ������ǰ���
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
        // <�����ײ�>      ::= procedure <id>'('[<ֵ����><id>{,<ֵ����><id>}]')';

        // prodecure
        ++m_pCurrSymbol;

        // <id>
        SafeCheck(ST_Identifier, 22);                   // [Error022] Identifier Expected
        // **id�ض�����**
        int level = 0;
        if (~GetIDIndex(m_pCurrSymbol->word, &level))
        {
            SafeCheck(ST_Error, 34);                    // [Error034] Identifier redefined
        }

        // **��ӹ���id**
        Identifier id = { "",ID_PROCEDURE,0,m_CurrLevel,
            static_cast<int>(m_ProgramInfo.instructions.size()) };
        strcpy_s(id.name, m_pCurrSymbol->word.c_str());
        m_CurrProcIndex = m_ProgramInfo.identifiers.size();
        m_ProgramInfo.identifiers.push_back(id);
        ++m_pCurrSymbol;

        // '('
        SafeCheck(ST_LeftParen, 24);                    // [Error024] '(' Expected.
        ++m_pCurrSymbol;

        // ***�ӹ��̵��βο�ʼΪ�ڲ�***
        ++m_CurrLevel;

        // [<ֵ����><id>{,<ֵ����><id>}]]')';
        if (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_Int)
        {
            // <ֵ����>
            SymbolType valueType = m_pCurrSymbol->symbolType;
            ++m_pCurrSymbol;

            // <id>
            SafeCheck(ST_Identifier, 22);                   // [Error022] Identifier Expected
            if (~GetIDIndex(m_pCurrSymbol->word, &level) && level == m_CurrLevel)
            {
                SafeCheck(ST_Error, 34);	// [Error034] Identifier redefined.
            }

            // ***��ӹ��̷���***
            id = { "",ID_PARAMETER | ID_VAR,0,m_CurrLevel,0 };
            strcpy_s(id.name, m_pCurrSymbol->word.c_str());
            switch (valueType)
            {
            case ST_Int:id.kind |= ID_INT;
                break;
            }


            m_ProgramInfo.identifiers.push_back(id);
            ++m_pCurrSymbol;


            // {,<ֵ����><id>}

            while (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_Comma)
            {
                ++m_pCurrSymbol;
                // <ֵ����>
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

                // ***��ӹ��̲�������***
                id = { "",ID_PARAMETER | ID_VAR,0,m_CurrLevel,0 };
                strcpy_s(id.name, m_pCurrSymbol->word.c_str());
                switch (valueType)
                {
                case ST_Int:id.kind |= ID_INT;
                    break;
                }

                // ***�β�����ƫ��***
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
        // <�ֳ���>        ::= <����˵������><����˵������><���>

        std::vector<Identifier> constants;
        std::vector<Identifier> variables;
        // <����˵������>
        ParseConstDesc(constants);
        // <����˵������>
        ParseVarDesc(variables);

        // **�������������λ�ã�ȷ����ʼջ��С
        int initStackUnit = 0;
        for (auto& id : variables)
        {
            // **����ƫ�ƣ���һ�������ĵ�ַ��0��ʼ
            if (!(m_ProgramInfo.identifiers.back().kind & ID_PROCEDURE))
            {
                id.offset = 1 + m_ProgramInfo.identifiers.back().offset;
            }
            ++initStackUnit;
            m_ProgramInfo.identifiers.push_back(id);
        }

        // **��ȡ�β���Ŀ
        int paramCount = 0;
        for (uint32_t i = m_ProgramInfo.identifiers.size() - 1; !(m_ProgramInfo.identifiers[i].kind & ID_PROCEDURE); --i)
        {
            if (m_ProgramInfo.identifiers[i].kind & ID_PARAMETER)
                ++paramCount;
        }

        //** ����ջ�ռ�**
        m_ProgramInfo.instructions.push_back({ Func_INT,0,paramCount });

        for (auto& id : constants)
        {
            m_ProgramInfo.identifiers.push_back(id);
        }

        // <���>
        ParseStatement();

        // ***���أ�������ջ�ռ�
        m_ProgramInfo.instructions.push_back({ Func_OPR,0,0 });

        // ***���̽�����������һ��
        --m_CurrLevel;
    }

    bool ProgramParser::ParseStatement()
    {
        try
        {
            // <���>          ::= <�����>|<��ֵ���>|<�������>|<whileѭ�����>|<forѭ�����>|<���̵������>|<�����>|<д���>|<�������>|<�������>

            // <�����>
            if (m_pCurrSymbol == m_pEndSymbol)
                return true;
            // <�������>
            else if (m_pCurrSymbol->symbolType == ST_Begin)
            {
                ParseComplexStat();
            }
            // <��ֵ���>
            else if (m_pCurrSymbol->symbolType == ST_Identifier || m_pCurrSymbol->symbolType == ST_SelfAdd || m_pCurrSymbol->symbolType == ST_SelfSub)
            {
                ParseAssignStat();
            }
            // <�������>
            else if (m_pCurrSymbol->symbolType == ST_If)
            {
                ParseConditionStat();
            }
            // <whileѭ�����>
            else if (m_pCurrSymbol->symbolType == ST_While)
            {
                ParseWhileLoopStat();
            }
            // <forѭ�����>
            else if (m_pCurrSymbol->symbolType == ST_For)
            {
                ParseForLoopStat();
            }
            // <���̵������>
            else if (m_pCurrSymbol->symbolType == ST_Call)
            {
                ParseCallStat();
            }
            // <�����>
            else if (m_pCurrSymbol->symbolType == ST_Read)
            {
                ParseReadStat();
            }
            // <д���>
            else if (m_pCurrSymbol->symbolType == ST_Write)
            {
                ParseWriteStat();
            }
            // <�������>
            else if (m_pCurrSymbol->symbolType == ST_Return)
            {
                ParseReturnStat();
            }
        }
        catch (std::exception)
        {
            if (m_pCurrSymbol != m_pEndSymbol)
            {
                //������ǰ���
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
        // <��ֵ���>      ::=[++|--] <id> | <id> [++|--] | :=|+=|-=|*=|/=|%= <���ʽ>


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
        // ***����id���������Ƿ����д��***
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
            // <���ʽ>
            ParseExpressionL1();
        }


        // �ǳ��渳ֵ����Ҫ����һ�ζ�Ԫ����
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


        // ***��ֵ�����������ں�����/�����������***
        m_ProgramInfo.instructions.insert(m_ProgramInfo.instructions.cend(), m_TempSelfSum.cbegin(), m_TempSelfSum.cend());
    }

    void ProgramParser::ParseComplexStat()
    {
        // <�������>      ::= begin <���>{;<���>} end
        
        // begin
        ++m_pCurrSymbol;

        // <���>
        ParseStatement();

        // {;<���>}
        while (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_SemiColon)
        {
            ++m_pCurrSymbol;

            // <���>
            ParseStatement();
        }

        // end
        SafeCheck(ST_End, 26);		// [Error026] Keyword 'end' expected.
        ++m_pCurrSymbol;
    }

    void ProgramParser::ParseConditionStat()
    {
        // <�������>      ::= if <����> then <���> {else if <����> then <���>}[else <���>]

        std::forward_list<int> trueList{};
        std::forward_list<int> falseList{};
        int trueLength = 0, falseLength = 0;
        // if 
        ++m_pCurrSymbol;

        // <����>
        ParseConditionL1();

        falseList.push_front(m_ProgramInfo.instructions.size());
        m_ProgramInfo.instructions.push_back({ Func_JPC,0,0 });         //��ַ������
        ++falseLength;

        // then
        SafeCheck(ST_Then, 27);		// [Error027] Keyword 'then' expected.
        ++m_pCurrSymbol;

        // ���
        ParseStatement();

        // {else if <����> then <���>}[else <���>]

        // else
        while (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_Else)
        {
            ++m_pCurrSymbol;

            trueList.push_front(m_ProgramInfo.instructions.size());
            m_ProgramInfo.instructions.push_back({ Func_JMP,0,0 });     //��ַ������
            ++trueLength;

            // if
            if (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_If)
            {
                ++m_pCurrSymbol;

                // <����>
                ParseConditionL1();

                falseList.push_front(m_ProgramInfo.instructions.size());
                m_ProgramInfo.instructions.push_back({ Func_JPC,0,0 });         //��ַ������
                ++falseLength;

                // then
                SafeCheck(ST_Then, 27);		// [Error027] Keyword 'then' expected.
                ++m_pCurrSymbol;

                //<���>
                ParseStatement();
            }
            else
            {
                ParseStatement();
                break;
            }
        }

        int currOffset = m_ProgramInfo.instructions.size();

        // ***��/������ַ����(�������ȿ��ܱ�������1)***
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
        // <whileѭ�����> ::= while <����> do <���>


        ++m_pCurrSymbol;

        int startLoopOffset = m_ProgramInfo.instructions.size();

        // <����>
        ParseConditionL1();

        // **�������ַ**
        int falseIndex = m_ProgramInfo.instructions.size();
        m_ProgramInfo.instructions.push_back({ Func_JPC,0,0 });


        // do
        SafeCheck(ST_Do, 28);           // [Error028] keyword'do' expected
        ++m_pCurrSymbol;

        ParseStatement();

        m_ProgramInfo.instructions.push_back({ Func_JMP,0,startLoopOffset });

        // ***�����ַ***
        m_ProgramInfo.instructions[falseIndex].mix = m_ProgramInfo.instructions.size();
    }

    void ProgramParser::ParseForLoopStat()
    {
        // <forѭ�����>   ::= for <id>:=<���ʽ> step <���ʽ> until <����> do <���>

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


        // <���ʽ>
        ParseExpressionL1();

        m_ProgramInfo.instructions.push_back({ Func_STO,m_CurrLevel - idLevel,idOffset });

        // step
        // ***����Ҫ���ж�������ִ�����飬�ڽ��и��£���Ҫ�����ⲿ��ָ��***
        SafeCheck(ST_Step, 29);                     // [Error029] Keyword 'step' expected.
        ++m_pCurrSymbol;
        
        uint32_t currInstructCount = m_ProgramInfo.instructions.size();
        std::vector<Instruction> temp{};

        // <���ʽ>
        ParseExpressionL1();

        // ***��ʱ������Щ���ʽ***
        temp.insert(temp.cbegin(), m_ProgramInfo.instructions.cbegin() + currInstructCount, m_ProgramInfo.instructions.cend());
        m_ProgramInfo.instructions.erase(m_ProgramInfo.instructions.cbegin() + currInstructCount);

        // until
        SafeCheck(ST_Until, 30);                    // [Error030] Keyword 'until' expected.
        ++m_pCurrSymbol;

        int startLoopOffset = m_ProgramInfo.instructions.size();

        // <����>
        ParseConditionL1();

        // **��������Ϊ��ʱ�˳�ѭ������������Ҫ�߼�ȡ��
        m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_NOT });
        // **�������ַ
        int falseIndex = m_ProgramInfo.instructions.size();
        m_ProgramInfo.instructions.push_back({ Func_JPC,0,0 });

        // do
        SafeCheck(ST_Do, 28);	// [Error028] Keyword 'do' expected.
        ++m_pCurrSymbol;

        // <���>
        ParseStatement();

        // **ִ��step**
        m_ProgramInfo.instructions.push_back({ Func_LOD,m_CurrLevel - idLevel,idOffset });
        m_ProgramInfo.instructions.insert(m_ProgramInfo.instructions.cend(),temp.cbegin(),temp.cend());
        m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_ADD });
        m_ProgramInfo.instructions.push_back({ Func_STO,m_CurrLevel - idLevel,idOffset });
        m_ProgramInfo.instructions.push_back({ Func_JMP,0,startLoopOffset });

        // **�����ַ***
        m_ProgramInfo.instructions[falseIndex].mix = m_ProgramInfo.instructions.size();
    }

    void ProgramParser::ParseReadStat()
    {
        // <�����>        ::= read '('<id>{,<id>}')'

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
        // **��֤id���������Ƿ����д��
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
            // **��֤id���������Ƿ����д��
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
        // <д���>        ::= write '('<���ʽ>{,<���ʽ>}')'

        // write
        ++m_pCurrSymbol;

        // '('
        SafeCheck(ST_LeftParen, 24);	// [Error024] '(' Expected.
        ++m_pCurrSymbol;

        // <���ʽ>
        ParseExpressionL1();

        m_ProgramInfo.instructions.push_back({ Func_OPR, 0, Opr_PRT });

        // {,<���ʽ>}

        while (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_Comma)
        {
            ++m_pCurrSymbol;

            // <���ʽ>
            ParseExpressionL1();

            m_ProgramInfo.instructions.push_back({ Func_OPR, 0, Opr_PRT });

        }


        // ')'
        SafeCheck(ST_RightParen, 25);		// [Error025] ')' Expected.
        ++m_pCurrSymbol;
    }

    void ProgramParser::ParseCallStat()
    {
        // <���̵������>  ::= call <id>'('[<���ʽ>{,<���ʽ>}]')'

        // call
        ++m_pCurrSymbol;
        int paramCount = 0;

        // id
        SafeCheck(ST_Identifier, 22);               // [Error022] Identifier Expected.
        const std::string& id = m_pCurrSymbol->word;
        int idLevel = 0;
        uint32_t pos = GetIDIndex(m_pCurrSymbol->word, &idLevel);
        // **��֤id���������Ƿ�Ϊ����
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

        // [<���ʽ>{,<���ʽ>}]')'

        // <���ʽ>
        if (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType != ST_RightParen)
        {
            ParseExpressionL1();
            ++paramCount;

            // {,<���ʽ>}

            // ,
            while (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_Comma)
            {
                ++m_pCurrSymbol;

                // <���ʽ>
                ParseExpressionL1();
                ++paramCount;
            }
        }

        // ')'
        SafeCheck(ST_RightParen, 25);		// [Error025] ')' Expected.
        ++m_pCurrSymbol;

        // *** �鿴������ʵ���β���Ŀ***
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


        // ***�˱�Ϊ�β��ó��ռ䣨���ı�ջ��ָ�룬���Ƴ���������ݣ�***
        m_ProgramInfo.instructions.push_back({ Func_POP,0,paramCount });

        // ***�����ú���***
        m_ProgramInfo.instructions.push_back({ Func_CAL,m_CurrLevel - idLevel,m_ProgramInfo.identifiers[pos].offset });
    }

    void ProgramParser::ParseReturnStat()
    {
        //<�������>      ::= return
        ++m_pCurrSymbol;
        m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_RET });
    }

    void ProgramParser::ParseConditionL1()
    {
        // <����>          ::= <��������>{<�߼���><��������>}

        // <��������>
        ParseConditionL2();

        // {<�߼���><��������>}
        if (m_pCurrSymbol == m_pEndSymbol || m_pCurrSymbol->symbolType != ST_LogicalOr)
            return;

        std::forward_list<int> trueList{};

        while (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_LogicalOr)
        {
            // �߼���
            ++m_pCurrSymbol;

            // ***��¼����***

            m_ProgramInfo.instructions.push_back({ Func_OPR,0,Opr_NOT });
            trueList.push_front(m_ProgramInfo.instructions.size());
            m_ProgramInfo.instructions.push_back({ Func_JPC,0,0 });

            // <��������>
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
        // <��������>      ::= <��������>{<�߼���><��������>}

        // <��������>
        ParseConditionL3();

        // {<�߼���><��������>}
        if (m_pCurrSymbol == m_pEndSymbol || m_pCurrSymbol->symbolType != ST_LogicalAnd)
            return;

        std::forward_list<int> falseList{};


        while (m_pCurrSymbol != m_pEndSymbol && m_pCurrSymbol->symbolType == ST_LogicalAnd)
        {
            // �߼���
            ++m_pCurrSymbol;

            // ***��¼����***
            falseList.push_front(m_ProgramInfo.instructions.size());
            m_ProgramInfo.instructions.push_back({ Func_JPC,0,0 });

            // <��������>
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
        // <��������>      ::= <�ļ�����>{!=|= <�ļ�����>}
        ParseConditionL4();


        // {!=|= <�ļ�����>}
        while (m_pCurrSymbol != m_pEndSymbol &&
            m_pCurrSymbol->symbolType == ST_NotEqual || m_pCurrSymbol->symbolType == ST_Equal)
        {
            // !=|=
            SymbolType type = m_pCurrSymbol->symbolType;
            ++m_pCurrSymbol;

            // <�ļ�����>
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
        // <�ļ�����>      ::= <���ʽ>{>|>=|<|<= <���ʽ>}
        ParseExpressionL1();

        // {>|>=|<|<= <���ʽ>}
        while (m_pCurrSymbol != m_pEndSymbol &&
            m_pCurrSymbol->symbolType == ST_Greater || m_pCurrSymbol->symbolType == ST_GreaterEqual ||
            m_pCurrSymbol->symbolType == ST_Less || m_pCurrSymbol->symbolType == ST_LessEqual)
        {
            // >|>=|<|<=
            SymbolType type = m_pCurrSymbol->symbolType;
            ++m_pCurrSymbol;

            // <���ʽ>
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
        // <���ʽ>        ::= <�������ʽ>{+|- <�������ʽ>}

        // <�������ʽ>
        ParseExpressionL2();

        while (m_pCurrSymbol != m_pEndSymbol &&
            m_pCurrSymbol->symbolType == ST_Plus || m_pCurrSymbol->symbolType == ST_Minus )
        {
            // +|-
            SymbolType type = m_pCurrSymbol->symbolType;
            ++m_pCurrSymbol;

            // <�������ʽ>
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
        // <�������ʽ>    ::= <�������ʽ>{*|/|% <�������ʽ>}

        // <�������ʽ>
        ParseExpressionL3();

        while (m_pCurrSymbol!=m_pEndSymbol&&
            m_pCurrSymbol->symbolType==ST_Multiply|| m_pCurrSymbol->symbolType==ST_Divide||m_pCurrSymbol->symbolType==ST_Mod)
        {
            // *|/|%
            SymbolType type = m_pCurrSymbol->symbolType;
            ++m_pCurrSymbol;

            // <�������ʽ>
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
        // <�������ʽ>    ::= [!|-|+]<�ļ����ʽ>
        
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

        // <�ļ����ʽ>
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

        //  <�ļ����ʽ>          ::= [++|--]<id>|<id>[++|--]| |<value>|'('<����>')'

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
            // ***����id���������Ƿ����д��***
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

            // ȡ������
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
            // ***������ָ��LIT,������ָ��LOD***
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

                //***�����ʱ������ָ��
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
        // '('<����>')'
        else if (m_pCurrSymbol->symbolType == ST_LeftParen)
        {
            ++m_pCurrSymbol;

            // <����>
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
        // ����ǰ�Ѿ�ɨ�������������У����ߵ�ǰ�����ķ���������ʵ�ʵĲ�һ�£����¼������Ϣ���׳��쳣
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
        // ���鵱ǰ�����Ƿ��и÷���
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


        // �����������Ƿ��и÷���
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

        // �����Ƿ�Ϊ����
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
