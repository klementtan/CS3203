// pql/parser.cpp

#include <cctype>
#include <cassert>
#include <unordered_set>

#include <zpr.h>

#include "util.h"
#include "exceptions.h"
#include "simple/parser.h"
#include "pql/parser/parser.h"

namespace pql::parser
{
    using PqlException = util::PqlException;
    using PqlSyntaxException = util::PqlSyntaxException;

    struct ParserState
    {
        Token next()
        {
            return getNextToken(m_stream);
        }

        Token next_keyword()
        {
            return getNextKeywordToken(m_stream);
        }

        Token peek_keyword() const
        {
            return peekNextKeywordToken(m_stream);
        }

        Token peek_one() const
        {
            return peekNextOneToken(m_stream);
        }

        std::vector<Token> peek_two() const
        {
            return peekNextTwoTokens(m_stream);
        }

        void assert_whitespace(int expected_whitespace, std::string msg)
        {
            int count = eatWhitespace(m_stream);
            if(count != expected_whitespace)
                throw PqlSyntaxException(
                    "pql::parser", "Expected {} whitespace but got {} instead. {}", expected_whitespace, count, msg);
        }

        bool hasDeclaration(zst::str_view name)
        {
            return m_query->declarations.hasDeclaration(name.str());
        }

        // note: the dummy declaration is here so that we don't propagate nullptrs all over the place.
        ast::Declaration* addDeclaration(zst::str_view name, ast::DESIGN_ENT type)
        {
            if(this->hasDeclaration(name))
            {
                m_query->setInvalid();
                util::logfmt("pql::parser", "duplicate declaration of '{}'", name);
                return &m_dummy_decl;
            }

            util::logfmt("pql::parser", "added declaration '{}' (type '{}')", name, ast::INV_DESIGN_ENT_MAP.at(type));
            return m_query->declarations.addDeclaration(name.str(), type);
        }

        ast::Declaration* getDeclaration(zst::str_view name)
        {
            if(!this->hasDeclaration(name))
            {
                m_query->setInvalid();
                util::logfmt("pql::parser", "use of undeclared synonym '{}'", name);
                return &m_dummy_decl;
            }
            return m_query->declarations.getDeclaration(name.str());
        }

        template <typename... Args>
        void setInvalid(const char* fmt, Args&&... args)
        {
            m_query->setInvalid();
            util::logfmt("pql::parser", "semantic error: {}", zpr::fwd(fmt, static_cast<Args&&>(args)...));
        }

        ParserState(zst::str_view input, ast::Query* query) : m_stream(input), m_query(query)
        {
            m_dummy_decl.name = "$uwu";
            m_dummy_decl.design_ent = ast::DESIGN_ENT::INVALID;
        }

    private:
        zst::str_view m_stream {};
        ast::Query* m_query {};
        ast::Declaration m_dummy_decl {};
    };






    // Clauses
    const Token KW_Boolean { "BOOLEAN", TT::Identifier };
    const std::vector<Token> KW_SuchThat = { { "such", TT::Identifier }, { "that", TT::Identifier } };
    const Token KW_Pattern { "pattern", TT::Identifier };

    // Design entities
    const Token KW_Stmt { "stmt", TT::Identifier };
    const Token KW_Assign { "assign", TT::Identifier };
    const Token KW_Variable { "variable", TT::Identifier };
    const Token KW_Constant { "constant", TT::Identifier };
    const Token KW_Procedure { "procedure", TT::Identifier };
    const Token KW_Read { "read", TT::Identifier };
    const Token KW_Print { "print", TT::Identifier };
    const Token KW_If { "if", TT::Identifier };
    const Token KW_Then { "then", TT::Identifier };
    const Token KW_Else { "else", TT::Identifier };
    const Token KW_Call { "call", TT::Identifier };
    const Token KW_While { "while", TT::Identifier };
    const std::unordered_set<Token> KW_DesignEntities { { KW_Stmt, KW_Assign, KW_Variable, KW_Constant, KW_Procedure,
        KW_Read, KW_Print, KW_If, KW_Then, KW_Else, KW_Call, KW_While } };

    // Relationships
    const Token KW_Follows { "Follows", TT::Identifier };
    const std::vector<Token> KW_FollowsT = { { "Follows", TT::Identifier }, { "*", TT::Asterisk } };
    const Token KW_Parent { "Parent", TT::Identifier };
    const std::vector<Token> KW_ParentT = { { "Parent", TT::Identifier }, { "*", TT::Asterisk } };
    const Token KW_Uses { "Uses", TT::Identifier };
    const Token KW_Modifies { "Modifies", TT::Identifier };
    const Token KW_Calls { "Calls", TT::Identifier };
    const std::vector<Token> KW_CallsT = { { "Calls", TT::Identifier }, { "*", TT::Asterisk } };


    // Attribute Names
    const Token KW_AttrName_ProcName { "procName", TT::Identifier };
    const Token KW_AttrName_VarName { "varName", TT::Identifier };
    const Token KW_AttrName_Value { "value", TT::Identifier };
    const Token KW_AttrName_StmtNum { "call", TT::Identifier };

    // Process the next token as a variable and insert it into declaration_list
    void parse_one_declaration(ParserState* ps, ast::DESIGN_ENT ent)
    {
        Token var = ps->next();

        if(var.type != TT::Identifier)
        {
            throw PqlSyntaxException(
                "pql::parser", "expected variable name to be an identifier but received '{}' instead.", var.text);
        }

        ps->addDeclaration(var.text, ent);
    }

    // Process the next tokens as the start of an entity declaration and insert declarations into declaration_list
    void parse_declarations(ParserState* ps)
    {
        auto check_semicolon = [](ParserState* ps) {
            if(ps->next() != TT::Semicolon)
                throw PqlSyntaxException("pql::parser", "expected semicolon after statement");
        };

        auto check_comma = [](ParserState* ps) {
            if(ps->next() != TT::Comma)
                throw PqlSyntaxException("parser", "expected semicolon after statement");
        };

        if(KW_DesignEntities.count(ps->peek_one()) == 0)
        {
            throw PqlSyntaxException("pql::parser",
                "Expected declarations to start with design-entity keyword instead of {}", ps->peek_one().text);
        }

        std::string ent_string { ps->next().text.str() };
        util::logfmt("pql::parser", "Parsing declaration with design_ent:{}", ent_string);

        if(ast::DESIGN_ENT_MAP.count(ent_string) == 0)
            util::logfmt("pql::parser", "Invalid entity provided in declaration {}", ent_string);

        ast::DESIGN_ENT ent = ast::DESIGN_ENT_MAP.find(ent_string)->second;
        parse_one_declaration(ps, ent);

        // Handle trailing additional var using `,`
        while(ps->peek_one().type == TT::Comma)
        {
            check_comma(ps);
            parse_one_declaration(ps, ent);
        }

        check_semicolon(ps);
    }


    ast::EntRef parse_ent_ref(ParserState* ps)
    {
        Token tok = ps->next();
        if(tok.type == TT::Underscore)
        {
            return ast::EntRef::ofWildcard();
        }
        else if(tok.type == TT::String)
        {
            // make sure it's a valid identifier
            auto name = tok.text.str();
            {
                bool fail = false;
                if(!std::isalpha(name[0]))
                    fail = true;

                for(char c : name)
                {
                    if(!std::isdigit(c) && !std::isalpha(c))
                    {
                        fail = true;
                        break;
                    }
                }

                if(fail)
                {
                    throw PqlSyntaxException(
                        "pql::parser", "Expected literal entity name to be an identifier instead of '{}'", name);
                }
            }

            return ast::EntRef::ofName(name);
        }
        else if(tok.type == TokenType::Identifier)
        {
            std::string var_name = tok.text.str();

            auto declaration = ps->getDeclaration(var_name);
            return ast::EntRef::ofDeclaration(declaration);
        }

        throw PqlSyntaxException("pql::parser", "Invalid entity ref starting with '{}'", tok.text);
    }

    ast::StmtRef parse_stmt_ref(ParserState* ps)
    {
        Token tok = ps->next();
        if(tok.type == TT::Underscore)
        {
            return ast::StmtRef::ofWildcard();
        }
        else if(tok.type == TT::Number)
        {
            size_t id = 0;
            for(char c : tok.text)
                id = 10 * id + (c - '0');

            return ast::StmtRef::ofStatementId(id);
        }
        else if(tok.type == TokenType::Identifier)
        {
            auto declaration = ps->getDeclaration(tok.text);
            return ast::StmtRef::ofDeclaration(declaration);
        }

        throw PqlSyntaxException("pql::parser", "Invalid stmt ref starting with {}", tok.text);
    }

    ast::ExprSpec parse_expr_spec(ParserState* ps)
    {
        ast::ExprSpec expr_spec {};

        bool is_subexpr = false;
        if(ps->peek_one() == TT::Underscore)
        {
            ps->next();
            is_subexpr = true;
        }

        if(auto estr = ps->peek_one(); estr == TT::String)
        {
            ps->next();
            expr_spec.expr = simple::parser::parseExpression(estr.text);
        }

        // '_' itself is valid as well, so don't expect '__'
        if(is_subexpr && expr_spec.expr != nullptr)
        {
            if(ps->next() != TT::Underscore)
                throw PqlSyntaxException("pql::parser", "expected '_' in subexpression pattern");
        }

        expr_spec.is_subexpr = is_subexpr;
        return expr_spec;
    }

    // the declaration has already been eaten.
    std::unique_ptr<ast::AssignPatternCond> parse_assign_pattern(ParserState* ps, ast::Declaration* assign_decl)
    {
        assert(assign_decl->design_ent == ast::DESIGN_ENT::ASSIGN);
        util::logfmt("pql::parser", "Parsing pattern clause with assignment condition {}", assign_decl->toString());

        auto pattern_cond = std::make_unique<ast::AssignPatternCond>();

        if(Token tok = ps->next(); tok != TT::LParen)
        {
            throw PqlSyntaxException(
                "pql::parser", "Expected '(' after synonym in Pattern clause instead of {}", tok.text);
        }

        pattern_cond->assignment_declaration = assign_decl;
        pattern_cond->ent = parse_ent_ref(ps);
        if(pattern_cond->ent.isDeclaration() &&
            pattern_cond->ent.declaration()->design_ent != ast::DESIGN_ENT::VARIABLE)
            throw PqlException("pql::parser", "synonym in first argument of assign pattern must be a variable");

        if(Token tok = ps->next(); tok != TT::Comma)
        {
            throw PqlSyntaxException("pql::parser", "Expected ',' after ent ref declaration instead of {}", tok.text);
        }
        pattern_cond->expr_spec = parse_expr_spec(ps);
        if(Token tok = ps->next(); tok != TT::RParen)
        {
            throw PqlSyntaxException(
                "pql::parser", "Expected ')' after expr spec in Pattern clause instead of {}", tok.text);
        }

        return pattern_cond;
    }

    std::unique_ptr<ast::IfPatternCond> parse_if_pattern(ParserState* ps, ast::Declaration* if_decl)
    {
        assert(if_decl->design_ent == ast::DESIGN_ENT::IF);
        util::logfmt("pql::parser", "Parsing pattern clause with if condition {}", if_decl->toString());

        auto pattern_cond = std::make_unique<ast::IfPatternCond>();

        if(Token tok = ps->next(); tok != TT::LParen)
            throw PqlSyntaxException(
                "pql::parser", "Expected '(' after synonym in Pattern clause instead of '{}'", tok.text);

        pattern_cond->if_declaration = if_decl;
        pattern_cond->ent = parse_ent_ref(ps);
        if(pattern_cond->ent.isDeclaration() &&
            pattern_cond->ent.declaration()->design_ent != ast::DESIGN_ENT::VARIABLE)
            throw PqlException("pql::parser", "synonym in first argument of if pattern must be a variable");

        if(Token tok = ps->next(); tok != TT::Comma)
            throw PqlSyntaxException("pql::parser", "Expected ',' after ent ref declaration instead of '{}'", tok.text);

        if(auto tok = ps->next(); tok != TT::Underscore)
            throw PqlSyntaxException("pql::parser", "second argument of if-pattern can only be '_'");

        if(auto tok = ps->next(); tok != TT::Comma)
            throw PqlSyntaxException("pql::parser", "if-pattern requires 3 arguments, expected ','");

        if(auto tok = ps->next(); tok != TT::Underscore)
            throw PqlSyntaxException("pql::parser", "third argument of if-pattern can only be '_'");

        if(Token tok = ps->next(); tok != TT::RParen)
            throw PqlSyntaxException(
                "pql::parser", "Expected ')' after expr spec in Pattern clause instead of '{}'", tok.text);

        return pattern_cond;
    }


    std::unique_ptr<ast::WhilePatternCond> parse_while_pattern(ParserState* ps, ast::Declaration* while_decl)
    {
        assert(while_decl->design_ent == ast::DESIGN_ENT::WHILE);
        util::logfmt("pql::parser", "Parsing pattern clause with while condition {}", while_decl->toString());

        auto pattern_cond = std::make_unique<ast::WhilePatternCond>();

        if(Token tok = ps->next(); tok != TT::LParen)
            throw PqlSyntaxException(
                "pql::parser", "Expected '(' after synonym in Pattern clause instead of '{}'", tok.text);

        pattern_cond->while_declaration = while_decl;
        pattern_cond->ent = parse_ent_ref(ps);
        if(pattern_cond->ent.isDeclaration() &&
            pattern_cond->ent.declaration()->design_ent != ast::DESIGN_ENT::VARIABLE)
            throw PqlException("pql::parser", "synonym in first argument of while pattern must be a variable");

        if(Token tok = ps->next(); tok != TT::Comma)
            throw PqlSyntaxException("pql::parser", "Expected ',' after ent ref declaration instead of '{}'", tok.text);

        if(auto tok = ps->next(); tok != TT::Underscore)
            throw PqlSyntaxException("pql::parser", "second argument of while-pattern can only be '_'");

        if(Token tok = ps->next(); tok != TT::RParen)
            throw PqlSyntaxException(
                "pql::parser", "Expected ')' after expr spec in Pattern clause instead of '{}'", tok.text);

        return pattern_cond;
    }







    static ast::PatternCl parse_pattern(ParserState* ps)
    {
        std::vector<std::unique_ptr<ast::PatternCond>> pattern_conds;

        if(Token tok = ps->next_keyword(); tok != TT::KW_Pattern)
            throw PqlSyntaxException("pql::parser", "Expected 'pattern' instead of '{}'", tok.text);

        Token declaration_tok = ps->next();
        auto pattern_decl = ps->getDeclaration(declaration_tok.text.str());

        if(pattern_decl->design_ent == ast::DESIGN_ENT::ASSIGN)
        {
            pattern_conds.push_back(parse_assign_pattern(ps, pattern_decl));
        }
        else if(pattern_decl->design_ent == ast::DESIGN_ENT::IF)
        {
            pattern_conds.push_back(parse_if_pattern(ps, pattern_decl));
        }
        else if(pattern_decl->design_ent == ast::DESIGN_ENT::WHILE)
        {
            pattern_conds.push_back(parse_while_pattern(ps, pattern_decl));
        }
        else
        {
            ps->setInvalid("invalid synonym type '{}' in pattern clause (can only have 'if', 'while', or 'assign'",
                ast::INV_DESIGN_ENT_MAP.at(pattern_decl->design_ent));
        }

        return ast::PatternCl { std::move(pattern_conds) };
    }

    std::unique_ptr<ast::FollowsT> parse_follows_t(ParserState* ps)
    {
        auto f = ps->next();
        ps->assert_whitespace(0, "There should be not white space between 'Follows' and '*'");
        auto star = ps->next();

        if(f.text != "Follows" || star != TT::Asterisk)
        {
            throw PqlSyntaxException("pql::parser", "FollowsT relationship condition should start with 'Follows*'");
        }

        auto follows_t = std::make_unique<ast::FollowsT>();
        if(Token tok = ps->next(); tok != TT::LParen)
        {
            throw PqlSyntaxException("pql::parser", "Expected '(' at the start of 'Follows*' instead of {}", tok.text);
        }
        follows_t->before = parse_stmt_ref(ps);
        if(Token tok = ps->next(); tok != TT::Comma)
        {
            throw PqlSyntaxException("pql::parser", "Expected ',' after declaring ent ref in Follows*");
        }
        follows_t->after = parse_stmt_ref(ps);
        if(Token tok = ps->next(); tok != TT::RParen)
        {
            throw PqlSyntaxException("pql::parser", "Expected ')' at the end of 'Follows*' instead of {}", tok.text);
        }

        return follows_t;
    }

    std::unique_ptr<ast::Follows> parse_follows(ParserState* ps)
    {
        if(ps->next().text != "Follows")
        {
            throw PqlSyntaxException("pql::parser", "FollowsT relationship condition should start with 'Follows*'");
        }
        auto follows = std::make_unique<ast::Follows>();
        if(Token tok = ps->next(); tok != TT::LParen)
        {
            throw PqlSyntaxException("pql::parser", "Expected '(' at the start of 'Follows' instead of {}", tok.text);
        }
        follows->directly_before = parse_stmt_ref(ps);
        if(Token tok = ps->next(); tok != TT::Comma)
        {
            throw PqlSyntaxException("pql::parser", "Expected ',' after declaring ent ref in Follows");
        }
        follows->directly_after = parse_stmt_ref(ps);
        if(Token tok = ps->next(); tok != TT::RParen)
        {
            throw PqlSyntaxException("pql::parser", "Expected ')' at the end of 'Follows' instead of {}", tok.text);
        }
        util::logfmt("pql::parser", "Parsed: {}", follows->toString());

        return follows;
    }

    std::unique_ptr<ast::ParentT> parse_parent_t(ParserState* ps)
    {
        auto p = ps->next();
        ps->assert_whitespace(0, "There should be no whitespace between 'Parent' and '*'");
        auto star = ps->next();

        if(p.text != "Parent" || star != TT::Asterisk)
        {
            throw PqlSyntaxException("pql::parser", "ParentT relationship condition should start with 'Parent*'");
        }
        auto parent_t = std::make_unique<ast::ParentT>();
        if(Token tok = ps->next(); tok != TT::LParen)
        {
            throw PqlSyntaxException("pql::parser", "Expected '(' at the start of 'Parent*' instead of {}", tok.text);
        }
        parent_t->ancestor = parse_stmt_ref(ps);
        if(Token tok = ps->next(); tok != TT::Comma)
        {
            throw PqlSyntaxException("pql::parser", "Expected ',' after declaring ent ref in Parent*");
        }
        parent_t->descendant = parse_stmt_ref(ps);
        if(Token tok = ps->next(); tok != TT::RParen)
        {
            throw PqlSyntaxException("pql::parser", "Expected ')' at the end of 'Parent*' instead of {}", tok.text);
        }

        return parent_t;
    }

    std::unique_ptr<ast::Parent> parse_parent(ParserState* ps)
    {
        if(ps->next().text != "Parent")
        {
            throw PqlSyntaxException("pql::parser", "Parent relationship condition should start with 'Parent'");
        }
        auto parent = std::make_unique<ast::Parent>();
        if(Token tok = ps->next(); tok != TT::LParen)
        {
            throw PqlSyntaxException("pql::parser", "Expected '(' at the start of 'Parent' instead of {}", tok.text);
        }
        parent->parent = parse_stmt_ref(ps);
        if(Token tok = ps->next(); tok != TT::Comma)
        {
            throw PqlSyntaxException("pql::parser", "Expected ',' after declaring ent ref in Parent");
        }
        parent->child = parse_stmt_ref(ps);
        if(Token tok = ps->next(); tok != TT::RParen)
        {
            throw PqlSyntaxException("pql::parser", "Expected ')' at the end of 'Parent' instead of {}", tok.text);
        }

        return parent;
    }

    std::unique_ptr<ast::CallsT> parse_calls_t(ParserState* ps)
    {
        auto c = ps->next();
        ps->assert_whitespace(0, "There should be no whitespace between 'Calls' and '*'");
        auto star = ps->next();

        if(c.text != "Calls" || star != TT::Asterisk)
        {
            throw PqlSyntaxException("pql::parser", "CallsT relationship condition should start with 'Calls*'");
        }
        auto calls_t = std::make_unique<ast::CallsT>();
        if(Token tok = ps->next(); tok != TT::LParen)
        {
            throw PqlSyntaxException("pql::parser", "Expected '(' at the start of 'Calls*' instead of {}", tok.text);
        }
        calls_t->caller = parse_ent_ref(ps);
        if(Token tok = ps->next(); tok != TT::Comma)
        {
            throw PqlSyntaxException("pql::parser", "Expected ',' after declaring ent ref in Calls*");
        }
        calls_t->proc = parse_ent_ref(ps);
        if(Token tok = ps->next(); tok != TT::RParen)
        {
            throw PqlSyntaxException("pql::parser", "Expected ')' at the end of 'Calls*' instead of {}", tok.text);
        }

        return calls_t;
    }

    std::unique_ptr<ast::Calls> parse_calls(ParserState* ps)
    {
        if(ps->next().text != "Calls")
        {
            throw PqlSyntaxException("pql::parser", "Calls relationship condition should start with 'Calls'");
        }
        auto calls = std::make_unique<ast::Calls>();
        if(Token tok = ps->next(); tok != TT::LParen)
        {
            throw PqlSyntaxException("pql::parser", "Expected '(' at the start of 'Calls' instead of {}", tok.text);
        }
        calls->caller = parse_ent_ref(ps);
        if(Token tok = ps->next(); tok != TT::Comma)
        {
            throw PqlSyntaxException("pql::parser", "Expected ',' after declaring ent ref in Calls");
        }
        calls->proc = parse_ent_ref(ps);
        if(Token tok = ps->next(); tok != TT::RParen)
        {
            throw PqlSyntaxException("pql::parser", "Expected ')' at the end of 'Calls' instead of {}", tok.text);
        }

        return calls;
    }


    bool is_next_stmt_ref(ParserState* ps)
    {
        Token tok = ps->peek_one();

        // all numbers are statement refs, and assume underscores are as well.
        if(tok.type == TT::Number || tok.type == TT::Underscore)
            return true;

        // only entityrefs can be strings
        if(tok.type == TT::String)
            return false;

        // Only ref to previously declared entity allowed
        if(tok.type != TT::Identifier)
        {
            throw PqlSyntaxException("pql::parser",
                "StmtRef,EntRef should start with number, underscore, '\"' or identifier instead of {}", tok.text);
        }

        auto decl = ps->getDeclaration(tok.text.str());

        // Check if the reference to previously declared entity is a stmt.
        return ast::kStmtDesignEntities.count(decl->design_ent) > 0;
    }

    std::unique_ptr<ast::Uses> parse_uses(ParserState* ps)
    {
        if(ps->next().text != "Uses")
        {
            throw PqlSyntaxException("pql::parser", "Uses relationship condition should start with 'Uses'");
        }

        if(Token tok = ps->next(); tok.type != TT::LParen)
        {
            throw PqlSyntaxException("pql::parser", "Expected '(' at the start of 'Uses' instead of {}", tok.text);
        }
        bool is_user_stmt_ref = is_next_stmt_ref(ps);

        if(is_user_stmt_ref)
        {
            auto user = parse_stmt_ref(ps);
            if(Token tok = ps->next(); tok != TT::Comma)
            {
                throw PqlSyntaxException("pql::parser", "Expected ',' after declaring stmt ref in Uses");
            }
            auto ent = parse_ent_ref(ps);
            if(Token tok = ps->next(); tok.type != TT::RParen)
            {
                throw PqlSyntaxException("pql::parser", "Expected ')' at the end of 'Uses' instead of {}", tok.text);
            }
            auto user_s = std::make_unique<ast::UsesS>();
            user_s->user = user;
            user_s->ent = ent;
            return user_s;
        }
        else
        {
            auto user = parse_ent_ref(ps);
            if(Token tok = ps->next(); tok != TT::Comma)
            {
                throw PqlSyntaxException("pql::parser", "Expected ',' after declaring ent ref in Uses");
            }
            auto ent = parse_ent_ref(ps);
            if(Token tok = ps->next(); tok.type != TT::RParen)
            {
                throw PqlSyntaxException("pql::parser", "Expected ')' at the end of 'Uses' instead of {}", tok.text);
            }
            auto user_p = std::make_unique<ast::UsesP>();
            user_p->user = user;
            user_p->ent = ent;
            return user_p;
        }
    }

    std::unique_ptr<ast::Modifies> parse_modifies(ParserState* ps)
    {
        if(ps->next().text != "Modifies")
        {
            throw PqlSyntaxException("pql::parser", "Modifies relationship condition should start with 'Modifies'");
        }

        if(Token tok = ps->next(); tok.type != TT::LParen)
        {
            throw PqlSyntaxException("pql::parser", "Expected '(' at the start of 'Modifies' instead of {}", tok.text);
        }
        bool is_modifier_stmt_ref = is_next_stmt_ref(ps);

        if(is_modifier_stmt_ref)
        {
            auto modifier = parse_stmt_ref(ps);
            if(Token tok = ps->next(); tok != TT::Comma)
            {
                throw PqlSyntaxException("pql::parser", "Expected ',' after declaring stmt ref in Modifies");
            }
            auto ent = parse_ent_ref(ps);
            if(Token tok = ps->next(); tok.type != TT::RParen)
            {
                throw PqlSyntaxException(
                    "pql::parser", "Expected ')' at the end of 'Modifies' instead of {}", tok.text);
            }
            auto modifies_s = std::make_unique<ast::ModifiesS>();
            modifies_s->modifier = modifier;
            modifies_s->ent = ent;
            return modifies_s;
        }
        else
        {
            auto modifier = parse_ent_ref(ps);
            if(Token tok = ps->next(); tok != TT::Comma)
            {
                throw PqlSyntaxException("pql::parser", "Expected ',' after declaring stmt ref in Modifies");
            }
            auto ent = parse_ent_ref(ps);
            if(Token tok = ps->next(); tok.type != TT::RParen)
            {
                throw PqlSyntaxException(
                    "pql::parser", "Expected ')' at the end of 'Modifies' instead of {}", tok.text);
            }
            auto modifies_s = std::make_unique<ast::ModifiesP>();
            modifies_s->modifier = modifier;
            modifies_s->ent = ent;
            return modifies_s;
        }
    }


    std::unique_ptr<ast::RelCond> parse_rel_cond(ParserState* ps)
    {
        std::vector<Token> rel_cond_toks = ps->peek_two();

        // Check relationship with 2 tokens first
        if(rel_cond_toks == KW_FollowsT)
            return parse_follows_t(ps);

        else if(rel_cond_toks == KW_ParentT)
            return parse_parent_t(ps);

        else if(rel_cond_toks == KW_CallsT)
            return parse_calls_t(ps);

        else if(rel_cond_toks[0] == KW_Follows)
            return parse_follows(ps);

        else if(rel_cond_toks[0] == KW_Parent)
            return parse_parent(ps);

        else if(rel_cond_toks[0] == KW_Uses)
            return parse_uses(ps);

        else if(rel_cond_toks[0] == KW_Modifies)
            return parse_modifies(ps);

        else if(rel_cond_toks[0] == KW_Calls)
            return parse_calls(ps);

        throw PqlSyntaxException("pql::parser", "Invalid relationship condition tokens: {}, {}", rel_cond_toks[0].text,
            rel_cond_toks[1].text);
    }

    ast::SuchThatCl parse_such_that(ParserState* ps)
    {
        auto such = ps->next();
        ps->assert_whitespace(1, "There should only be 1 whitespace between 'such' and 'that'");
        auto that = ps->next();

        if(such.text != "such" || that.text != "that")
            throw PqlSyntaxException("pql::parser", "Such That clause should start with 'such that'");

        util::logfmt("pql::parser", "Parsing such that clause.");
        ast::SuchThatCl such_that {};

        // TODO(iteration 2): Handle AND condition here
        such_that.rel_conds.push_back(parse_rel_cond(ps));

        util::logfmt("pql::parser", "Complete parsing such that clause: {}", such_that.toString());
        return such_that;
    }

    static void validate_attr_name(const ast::AttrRef& attr_ref)
    {
        static const std::unordered_map<ast::AttrName, std::unordered_set<ast::DESIGN_ENT>>
            permitted_design_entities = { { ast::AttrName::kProcName,
                                              { ast::DESIGN_ENT::PROCEDURE, ast::DESIGN_ENT::CALL } },
                { ast::AttrName::kVarName,
                    { ast::DESIGN_ENT::VARIABLE, ast::DESIGN_ENT::READ, ast::DESIGN_ENT::PRINT } },
                { ast::AttrName::kValue, { ast::DESIGN_ENT::CONSTANT } },
                { ast::AttrName::kStmtNum,
                    { ast::DESIGN_ENT::STMT, ast::DESIGN_ENT::READ, ast::DESIGN_ENT::PRINT, ast::DESIGN_ENT::CALL,
                        ast::DESIGN_ENT::WHILE, ast::DESIGN_ENT::IF, ast::DESIGN_ENT::ASSIGN } } };
        ast::AttrName attr_name = attr_ref.attr_name;
        if(!attr_ref.decl)
            throw PqlException("pql::parser", "Invalid AttrRef: All AttrRef should have a declaration");
        ast::DESIGN_ENT design_ent = attr_ref.decl->design_ent;
        if(permitted_design_entities.count(attr_name) == 0)
            throw PqlSyntaxException(
                "pql::parser", "Invalid AttrName {} provided", ast::InvAttrNameMap.find(attr_name)->second);
        if(permitted_design_entities.find(attr_name)->second.count(design_ent) == 0)
            throw PqlException("pql::parser", "Invalid design_ent:{} does not contain AttrName {}",
                ast::INV_DESIGN_ENT_MAP.find(design_ent)->second, ast::InvAttrNameMap.find(attr_name)->second);
    }

    ast::Elem parse_elem(ParserState* ps)
    {
        Token decl_tok = ps->next();
        if(decl_tok.type != TT::Identifier)
            throw PqlSyntaxException(
                "pql::parser", "Expected identifier as the first token of an Element, got '{}'", decl_tok.text);

        auto decl = ps->getDeclaration(decl_tok.text.str());

        if(ps->peek_one() == TT::Dot)
        {
            // Eat dot
            ps->next();

            std::vector<Token> attr_toks = ps->peek_two();
            if(attr_toks[0].type != TT::Identifier)
                throw PqlSyntaxException(
                    "pql::parser", "Mutli Element tuple: Expected first token after '.' to be a identifier.");

            std::string attr_name_string;

            if(attr_toks[1].type == TT::HashTag)
            {
                auto stmt = ps->next();
                // should not have any whitespace between
                ps->assert_whitespace(0, "should not have any whitespace between the 'stmt' and '#'");
                auto hash_tag = ps->next();
                if(stmt.text != "stmt" || hash_tag.type != TT::HashTag)
                    throw PqlSyntaxException("pql::parser", "Invalid \"{}{}\" attribute name expected 'stmt#' instead.",
                        stmt.text.str(), hash_tag.text.str());
                attr_name_string = "stmt#";
            }
            else
            {
                attr_name_string = ps->next().text.str();
            }
            auto it = pql::ast::AttrNameMap.find(attr_name_string);
            if(it == pql::ast::AttrNameMap.end())
                throw PqlSyntaxException("pql::parser", "Invalid attrName: {}", attr_name_string);

            ast::AttrRef attr_ref { decl, it->second };
            validate_attr_name(attr_ref);
            return ast::Elem::ofAttrRef(attr_ref);
        }
        else
        {
            return ast::Elem::ofDeclaration(decl);
        }
    }

    std::vector<ast::Elem> parse_tuple(ParserState* ps)
    {
        // Handle: elem
        if(ps->peek_one() != TT::LAngle)
        {
            util::logfmt("pql::parser", "Parsing tuple as a single element without '<>'");
            return { parse_elem(ps) };
        }

        // Handle: '<' elem (',' elem)*'>'
        if(Token tok = ps->next(); tok != TT::LAngle)
            throw PqlSyntaxException(
                "pql::parser", "Multiple elem tuple should start with `<` instead of {}", tok.text);
        std::vector<ast::Elem> ret;
        while(ps->peek_one() != TT::RAngle &&
              // prevent infinite loop
              ps->peek_one() != TT::EndOfFile)
        {
            if(!ret.empty() && ps->peek_one() != TT::Comma)
            {
                throw PqlSyntaxException("pql::parser",
                    "Multiple element tuple should be comma separated instead of {}.", ps->peek_one().text);
            }
            if(ret.empty() && ps->peek_one() == TT::Comma)
            {
                throw PqlSyntaxException("pql::parser", "Multiple element tuple should not start with `,`");
            }
            // Should be safe to eat comma if it is present
            if(ps->peek_one() == TT::Comma)
                ps->next();
            ast::Elem elem = parse_elem(ps);
            util::logfmt("pql::ast", "Parsed new Elem {}", elem.toString());
            ret.push_back(elem);
        }
        if(Token tok = ps->next(); tok != TT::RAngle)
        {
            throw PqlSyntaxException("pql::parser", "Multiple elem tuple should end with `>` instead of {}", tok.text);
        }
        if(ret.empty())
            throw PqlSyntaxException("pql::parser", "Tuple in result clause cannot be empty");
        return ret;
    }

    ast::ResultCl parse_result(ParserState* ps)
    {
        if(ps->peek_one() == KW_Boolean)
        {
            ps->next();
            return ast::ResultCl::ofBool();
        }
        else
        {
            return ast::ResultCl::ofTuple(parse_tuple(ps));
        }
    }

    static ast::Select parse_select(ParserState* ps)
    {
        Token select_tok = ps->next_keyword();
        if(select_tok != TT::KW_Select)
        {
            throw PqlSyntaxException(
                "pql::parser", "Select clauses should start with `Select` instead of {}", select_tok.text);
        }

        ast::ResultCl result = parse_result(ps);

        util::logfmt("pql::parser", "Result for Select clause: {}", result.toString());

        ast::Select select {};
        select.result = result;

        // std::vector<Token> clause_tok = ps->peek_keyword();
        for(Token t; (t = ps->peek_keyword()) != TT::EndOfFile;)
        {
            if(t == TT::KW_Pattern)
            {
                util::logfmt("pql::parser", "Parsing pattern clause");
                select.pattern = parse_pattern(ps);
            }
            else if(t == TT::KW_SuchThat)
            {
                util::logfmt("pql::parser", "Parsing such that clause");
                select.such_that = parse_such_that(ps);
            }
            else if(t == TT::KW_With)
            {
                // TODO: parse 'with'
            }
            else
            {
                throw PqlSyntaxException("pql::parser", "unexpected token '{}' in Select", t.text);
            }
        }

        util::logfmt("pql::parser", "Completed parsing Select clause :{}", select.toString());
        return select;
    }



    std::unique_ptr<ast::Query> parsePQL(zst::str_view input)
    {
        util::logfmt("pql::parer", "Parsing input {}", input);
        auto query = std::make_unique<ast::Query>();
        auto ps = ParserState(input, query.get());

        bool found_select = false;
        for(Token t; (t = ps.peek_keyword()) != TT::EndOfFile;)
        {
            if(t == TT::KW_Select)
            {
                util::logfmt("pql::parser", "parsing Select");
                query->select = parse_select(&ps);

                found_select = true;
                if(ps.peek_one() != TT::EndOfFile)
                {
                    throw PqlSyntaxException("pql::parser",
                        "Query should end after a single select clause instead of '{}'", ps.peek_one().text);
                }
            }
            else
            {
                util::logfmt("pql::parser", "parsing declaration");
                parse_declarations(&ps);
            }
        }

        if(!found_select)
            throw PqlSyntaxException("pql::parser", "All queries should contain a select clause");

        util::logfmt("pql::parser", "Completed parsing AST: {}", query->toString());
        return query;
    }
}
