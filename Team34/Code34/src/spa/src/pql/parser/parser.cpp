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
    using SyntaxError = util::PqlSyntaxException;

    static ast::Declaration _dummy_decl { "$uwu", ast::DESIGN_ENT::INVALID };

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

        Token peek() const
        {
            return peekNextToken(m_stream);
        }

        Token expect(TokenType tt)
        {
            if(auto tok = this->next(); tok != tt)
            {
                throw SyntaxError("expected {}, found {} instead", tokenTypeString(tt), tokenTypeString(tok));
            }
            else
            {
                return tok;
            }
        }

        Token expect_keyword(TokenType tt)
        {
            if(auto tok = this->next_keyword(); tok != tt)
            {
                throw SyntaxError("expected {}, found {} instead", tokenTypeString(tt), tokenTypeString(tok));
            }
            else
            {
                return tok;
            }
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
                return &_dummy_decl;
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
                return &_dummy_decl;
            }
            return m_query->declarations.getDeclaration(name.str());
        }

        template <typename... Args>
        void setInvalid(const char* fmt, Args&&... args)
        {
            m_query->setInvalid();
            util::logfmt("pql::parser", "semantic error: {}", zpr::fwd(fmt, static_cast<Args&&>(args)...));
        }

        ParserState(zst::str_view input, ast::Query* query) : m_stream(input), m_query(query) { }

    private:
        zst::str_view m_stream {};
        ast::Query* m_query {};
    };


    // Process the next token as a variable and insert it into declaration_list
    static void parse_one_declaration(ParserState* ps, ast::DESIGN_ENT ent)
    {
        auto var = ps->expect(TT::Identifier);
        ps->addDeclaration(var.text, ent);
    }

    // Process the next tokens as the start of an entity declaration and insert declarations into declaration_list
    static void parse_declarations(ParserState* ps)
    {
        std::string ent_string {};
        if(auto tok = ps->peek_keyword(); tok == TT::KW_ProgLine)
            ent_string = ps->next_keyword().text.str();
        else
            ent_string = ps->expect(TT::Identifier).text.str();

        util::logfmt("pql::parser", "Parsing declaration with design_ent:{}", ent_string);

        if(ast::DESIGN_ENT_MAP.count(ent_string) == 0)
            throw SyntaxError("Invalid entity '{}' provided in declaration", ent_string);

        ast::DESIGN_ENT ent = ast::DESIGN_ENT_MAP.find(ent_string)->second;
        parse_one_declaration(ps, ent);

        // Handle trailing additional var using `,`
        while(ps->peek().type == TT::Comma)
        {
            ps->expect(TT::Comma);
            parse_one_declaration(ps, ent);
        }

        ps->expect(TT::Semicolon);
    }

    static ast::EntRef parse_ent_ref(ParserState* ps)
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
                    throw SyntaxError("Expected literal entity name to be an identifier instead of '{}'", name);
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

        throw SyntaxError("Invalid entity ref starting with '{}'", tok.text);
    }

    static ast::StmtRef parse_stmt_ref(ParserState* ps)
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

        throw SyntaxError("Invalid stmt ref starting with {}", tok.text);
    }

    static ast::ExprSpec parse_expr_spec(ParserState* ps)
    {
        ast::ExprSpec expr_spec {};

        bool is_subexpr = false;
        if(ps->peek() == TT::Underscore)
        {
            ps->next();
            is_subexpr = true;
        }

        if(auto estr = ps->peek(); estr == TT::String)
        {
            ps->next();
            expr_spec.expr = simple::parser::parseExpression(estr.text);
        }

        // '_' itself is valid as well, so don't expect '__'
        if(is_subexpr && expr_spec.expr != nullptr)
            ps->expect(TT::Underscore);

        expr_spec.is_subexpr = is_subexpr;
        return expr_spec;
    }

    // the declaration has already been eaten.
    static std::unique_ptr<ast::AssignPatternCond> parse_assign_pattern(ParserState* ps, ast::Declaration* assign_decl)
    {
        assert(assign_decl->design_ent == ast::DESIGN_ENT::ASSIGN);
        util::logfmt("pql::parser", "Parsing pattern clause with assignment condition {}", assign_decl->toString());

        ps->expect(TT::LParen);

        auto pattern_cond = std::make_unique<ast::AssignPatternCond>();
        pattern_cond->assignment_declaration = assign_decl;

        auto ent_ref = parse_ent_ref(ps);
        pattern_cond->ent = ent_ref;

        if(ent_ref.isDeclaration() && ent_ref.declaration()->design_ent != ast::DESIGN_ENT::VARIABLE)
            ps->setInvalid("synonym in first argument of assign pattern must be a variable");

        ps->expect(TT::Comma);

        pattern_cond->expr_spec = parse_expr_spec(ps);

        ps->expect(TT::RParen);
        return pattern_cond;
    }

    static std::unique_ptr<ast::PatternCond> parse_if_while_pattern(ParserState* ps, ast::Declaration* decl)
    {
        assert(decl->design_ent == ast::DESIGN_ENT::IF || decl->design_ent == ast::DESIGN_ENT::WHILE);
        util::logfmt("pql::parser", "Parsing pattern clause with if/while condition {}", decl->toString());

        ps->expect(TT::LParen);

        auto ent_ref = parse_ent_ref(ps);

        if(ent_ref.isDeclaration() && ent_ref.declaration()->design_ent != ast::DESIGN_ENT::VARIABLE)
            ps->setInvalid("synonym in first argument of if/while pattern must be a variable");

        if(decl->design_ent == ast::DESIGN_ENT::IF)
        {
            // needs 2 of these
            ps->expect(TT::Comma);
            ps->expect(TT::Underscore);
            ps->expect(TT::Comma);
            ps->expect(TT::Underscore);
            ps->expect(TT::RParen);

            auto ret = std::make_unique<ast::IfPatternCond>();
            ret->if_declaration = decl;
            ret->ent = ent_ref;
            return ret;
        }
        else if(decl->design_ent == ast::DESIGN_ENT::WHILE)
        {
            ps->expect(TT::Comma);
            ps->expect(TT::Underscore);
            ps->expect(TT::RParen);

            auto ret = std::make_unique<ast::WhilePatternCond>();
            ret->while_declaration = decl;
            ret->ent = ent_ref;
            return ret;
        }
        else
        {
            assert(false && "unreachable");
        }
    }

    static void parse_pattern(ParserState* ps, ast::Select* select)
    {
        ps->expect_keyword(TT::KW_Pattern);

        do
        {
            Token declaration_tok = ps->next();

            auto pattern_decl = ps->getDeclaration(declaration_tok.text);
            auto decl_ent = pattern_decl->design_ent;

            if(decl_ent == ast::DESIGN_ENT::ASSIGN)
            {
                select->patterns.push_back(parse_assign_pattern(ps, pattern_decl));
            }
            else if(decl_ent == ast::DESIGN_ENT::IF || decl_ent == ast::DESIGN_ENT::WHILE)
            {
                select->patterns.push_back(parse_if_while_pattern(ps, pattern_decl));
            }
            else
            {
                // TODO: we won't be able to parse the rest of the pattern without knowing the correct
                // type of the declaration. should this be a syntactic error?
                ps->setInvalid("invalid synonym type '{}' in pattern clause (can only have 'if', 'while', or 'assign'",
                    ast::INV_DESIGN_ENT_MAP.at(decl_ent));
            }

        } while(ps->peek_keyword() == TT::KW_And ? (ps->next_keyword(), true) : false);
    }

    // parses relations where that are not Uses/Modifies (ie. which are not overloaded based on the type)
    template <typename RelAst, typename LeftRefType, typename RightRefType>
    static std::unique_ptr<RelAst> parse_relation(
        ParserState* ps, TokenType kw_tok, LeftRefType RelAst::*left_ref, RightRefType RelAst::*right_ref)
    {
        static_assert(std::is_same_v<LeftRefType, ast::EntRef> || std::is_same_v<LeftRefType, ast::StmtRef>);
        static_assert(std::is_same_v<RightRefType, ast::EntRef> || std::is_same_v<RightRefType, ast::StmtRef>);

        ps->expect_keyword(kw_tok);
        ps->expect(TT::LParen);

        auto relation = std::make_unique<RelAst>();

        if constexpr(std::is_same_v<LeftRefType, ast::EntRef>)
            relation.get()->*left_ref = parse_ent_ref(ps);
        else
            relation.get()->*left_ref = parse_stmt_ref(ps);

        ps->expect(TT::Comma);

        if constexpr(std::is_same_v<RightRefType, ast::EntRef>)
            relation.get()->*right_ref = parse_ent_ref(ps);
        else
            relation.get()->*right_ref = parse_stmt_ref(ps);

        ps->expect(TT::RParen);
        return relation;
    }

    static bool is_next_stmt_ref(ParserState* ps)
    {
        Token tok = ps->peek();

        // all numbers are statement refs, and assume underscores are as well.
        if(tok.type == TT::Number || tok.type == TT::Underscore)
            return true;

        // only entityrefs can be strings
        if(tok.type == TT::String)
            return false;

        // Only ref to previously declared entity allowed
        if(tok.type != TT::Identifier)
        {
            throw SyntaxError(
                "StmtRef,EntRef should start with number, underscore, '\"' or identifier instead of {}", tok.text);
        }

        auto decl = ps->getDeclaration(tok.text.str());

        // Check if the reference to previously declared entity is a stmt.
        return ast::kStmtDesignEntities.count(decl->design_ent) > 0;
    }

    static std::unique_ptr<ast::RelCond> parse_uses_modifies(ParserState* ps)
    {
        bool is_uses = ps->peek_keyword() == TT::KW_Uses;
        if(auto t = ps->next_keyword(); t != TT::KW_Uses && t != TT::KW_Modifies)
            throw SyntaxError("expected 'Uses' or 'Modifies', found '{}' instead", t.text);

        ps->expect(TT::LParen);

        if(is_next_stmt_ref(ps))
        {
            auto stmt = parse_stmt_ref(ps);
            ps->expect(TT::Comma);

            auto ent = parse_ent_ref(ps);
            ps->expect(TT::RParen);

            if(is_uses)
            {
                auto uses = std::make_unique<ast::UsesS>();
                uses->user = stmt;
                uses->ent = ent;
                return uses;
            }
            else
            {
                auto modifies = std::make_unique<ast::ModifiesS>();
                modifies->modifier = stmt;
                modifies->ent = ent;
                return modifies;
            }
        }
        else
        {
            auto ent1 = parse_ent_ref(ps);
            ps->expect(TT::Comma);

            auto ent2 = parse_ent_ref(ps);
            ps->expect(TT::RParen);

            if(is_uses)
            {
                auto uses = std::make_unique<ast::UsesP>();
                uses->user = ent1;
                uses->ent = ent2;
                return uses;
            }
            else
            {
                auto modifies = std::make_unique<ast::ModifiesP>();
                modifies->modifier = ent1;
                modifies->ent = ent2;
                return modifies;
            }
        }
    }

    static std::unique_ptr<ast::RelCond> parse_rel_cond(ParserState* ps)
    {
        using namespace ast;
        auto rel_tok = ps->peek_keyword();
        if(rel_tok == TT::KW_Follows)
            return parse_relation(ps, rel_tok, &Follows::directly_before, &Follows::directly_after);

        else if(rel_tok == TT::KW_FollowsStar)
            return parse_relation(ps, rel_tok, &FollowsT::before, &FollowsT::after);

        else if(rel_tok == TT::KW_Parent)
            return parse_relation(ps, rel_tok, &Parent::parent, &Parent::child);

        else if(rel_tok == TT::KW_ParentStar)
            return parse_relation(ps, rel_tok, &ParentT::ancestor, &ParentT::descendant);

        else if(rel_tok == TT::KW_Calls)
            return parse_relation(ps, rel_tok, &Calls::caller, &Calls::proc);

        else if(rel_tok == TT::KW_CallsStar)
            return parse_relation(ps, rel_tok, &CallsT::caller, &CallsT::proc);

        else if(rel_tok == TT::KW_Next)
            return parse_relation(ps, rel_tok, &Next::first, &Next::second);

        else if(rel_tok == TT::KW_NextStar)
            return parse_relation(ps, rel_tok, &NextT::first, &NextT::second);

        else if(rel_tok == TT::KW_Uses || rel_tok == TT::KW_Modifies)
            return parse_uses_modifies(ps);

        throw SyntaxError("Invalid relationship condition '{}'", rel_tok.text);
    }

    static void parse_such_that(ParserState* ps, ast::Select* select)
    {
        ps->expect_keyword(TT::KW_SuchThat);

        util::logfmt("pql::parser", "Parsing such that clause.");

        select->relations.push_back(parse_rel_cond(ps));
        while(ps->peek_keyword() == TT::KW_And)
        {
            ps->next_keyword();
            select->relations.push_back(parse_rel_cond(ps));
        }
    }


    static ast::Elem validate_attr_name(ParserState* ps, ast::AttrRef attr_ref)
    {
        using namespace ast;

        // clang-format off
        static const std::unordered_map<AttrName, std::unordered_set<DESIGN_ENT>> permitted_design_entities = {
                { AttrName::kProcName, { DESIGN_ENT::PROCEDURE, DESIGN_ENT::CALL } },
                { AttrName::kVarName, { DESIGN_ENT::VARIABLE, DESIGN_ENT::READ, DESIGN_ENT::PRINT } },
                { AttrName::kValue, { DESIGN_ENT::CONSTANT } },
                { AttrName::kStmtNum, {
                    DESIGN_ENT::STMT, DESIGN_ENT::READ,
                    DESIGN_ENT::PRINT, DESIGN_ENT::CALL,
                    DESIGN_ENT::WHILE, DESIGN_ENT::IF, DESIGN_ENT::ASSIGN }
                }
            };
        // clang-format on

        // note: if the declaration did not exist, the dummy declaration will be used
        // this ensures we are not dealing in nullptrs unnecessarily.
        ast::AttrName attr_name = attr_ref.attr_name;
        assert(attr_ref.decl);

        auto design_ent = attr_ref.decl->design_ent;

        // -- parsing should have already thrown a syntax error if the attribute is bogus
        auto it = permitted_design_entities.find(attr_name);
        assert(it != permitted_design_entities.end());

        if(it->second.count(design_ent) == 0)
        {
            ps->setInvalid("entity '{}' does not contain attribute '{}'", ast::INV_DESIGN_ENT_MAP.at(design_ent),
                ast::InvAttrNameMap.at(attr_name));
        }

        return ast::Elem::ofAttrRef(attr_ref);
    }

    static ast::Elem parse_elem(ParserState* ps)
    {
        auto decl_tok = ps->expect(TT::Identifier);
        auto decl = ps->getDeclaration(decl_tok.text.str());

        if(ps->peek() == TT::Dot)
        {
            // Eat dot
            ps->next();

            auto attr = ps->next_keyword();
            if(attr == TT::KW_StmtNum)
                return validate_attr_name(ps, ast::AttrRef { decl, ast::AttrName::kStmtNum });

            else if(attr == TT::KW_Value)
                return validate_attr_name(ps, ast::AttrRef { decl, ast::AttrName::kValue });

            else if(attr == TT::KW_VarName)
                return validate_attr_name(ps, ast::AttrRef { decl, ast::AttrName::kVarName });

            else if(attr == TT::KW_ProcName)
                return validate_attr_name(ps, ast::AttrRef { decl, ast::AttrName::kProcName });

            else
                throw SyntaxError("Invalid attribute '{}'", attr.text);
        }
        else
        {
            return ast::Elem::ofDeclaration(decl);
        }
    }




    static ast::WithCondRef parse_with_cond_ref(ParserState* ps)
    {
        // each 'ref' can either be a string, an integer, a declaration (synonym), or a dotop
        if(ps->peek() == TT::String)
        {
            return ast::WithCondRef::ofString(ps->next().text.str());
        }
        else if(ps->peek() == TT::Number)
        {
            auto num = ps->next().text.str();
            return ast::WithCondRef::ofNumber(std::move(num));
        }
        else
        {
            // reuse 'Elem' parsing.
            auto tmp_elem = parse_elem(ps);
            if(tmp_elem.isDeclaration())
            {
                if(tmp_elem.declaration()->design_ent != ast::DESIGN_ENT::PROG_LINE)
                    ps->setInvalid("only 'prog_line' synonyms can be used in a 'with' without an attr_ref");

                // desugar prog_line references here to just be .stmt#, so we don't have
                // to deal with two different things (stmt.stmt# and prog_line) that serve
                // the same purpose. after all, prog_line and stmt are interchangeable.

                return ast::WithCondRef::ofAttrRef(ast::AttrRef { tmp_elem.declaration(), ast::AttrName::kStmtNum });
            }
            else
            {
                return ast::WithCondRef::ofAttrRef(tmp_elem.attrRef());
            }
        }
    }

    static std::unique_ptr<ast::WithCond> parse_with_cond(ParserState* ps)
    {
        auto with = std::make_unique<ast::WithCond>();
        with->lhs = parse_with_cond_ref(ps);

        ps->expect(TT::Equal);

        with->rhs = parse_with_cond_ref(ps);
        return with;
    }




    static void parse_with(ParserState* ps, ast::Select* select)
    {
        ps->expect_keyword(TT::KW_With);

        select->withs.push_back(parse_with_cond(ps));
        while(ps->peek_keyword() == TT::KW_And)
        {
            ps->next_keyword();
            select->withs.push_back(parse_with_cond(ps));
        }
    }






    static std::vector<ast::Elem> parse_tuple(ParserState* ps)
    {
        // Handle: elem
        if(ps->peek() != TT::LAngle)
        {
            util::logfmt("pql::parser", "Parsing tuple as a single element without '<>'");
            return { parse_elem(ps) };
        }

        // Handle: '<' elem (',' elem)*'>'
        ps->expect(TT::LAngle);

        std::vector<ast::Elem> ret {};
        while(true)
        {
            ast::Elem elem = parse_elem(ps);
            util::logfmt("pql::ast", "Parsed new Elem {}", elem.toString());

            ret.push_back(elem);

            if(ps->peek() == TT::Comma)
            {
                ps->next();
            }
            else if(ps->peek() == TT::RAngle)
            {
                break;
            }
            else
            {
                throw SyntaxError("expected either ',' or '>' in tuple, found '{}' instead", ps->peek().text);
            }
        }

        ps->expect(TT::RAngle);
        if(ret.empty())
            throw SyntaxError("Tuple in result clause cannot be empty");

        return ret;
    }

    static ast::Select parse_select(ParserState* ps)
    {
        ps->expect_keyword(TT::KW_Select);

        ast::ResultCl result = [ps]() -> auto
        {
            if(auto tok = ps->peek(); tok == TT::Identifier && tok.text == "BOOLEAN")
            {
                ps->next();
                return ast::ResultCl::ofBool();
            }
            else
            {
                return ast::ResultCl::ofTuple(parse_tuple(ps));
            }
        }
        ();


        util::logfmt("pql::parser", "Result for Select clause: {}", result.toString());

        ast::Select select {};
        select.result = result;

        for(Token t; (t = ps->peek_keyword()) != TT::EndOfFile;)
        {
            if(t == TT::KW_Pattern)
            {
                util::logfmt("pql::parser", "Parsing pattern clause");
                parse_pattern(ps, &select);
            }
            else if(t == TT::KW_SuchThat)
            {
                util::logfmt("pql::parser", "Parsing such that clause");
                parse_such_that(ps, &select);
            }
            else if(t == TT::KW_With)
            {
                util::logfmt("pql::parser", "Parsing with clause");
                parse_with(ps, &select);
            }
            else
            {
                throw SyntaxError("unexpected token '{}' in Select", t.text);
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
                if(ps.peek() != TT::EndOfFile)
                {
                    throw SyntaxError("Query should end after a single select clause instead of '{}'", ps.peek().text);
                }
            }
            else
            {
                util::logfmt("pql::parser", "parsing declaration");
                parse_declarations(&ps);
            }
        }

        if(!found_select)
            throw SyntaxError("All queries should contain a select clause");

        util::logfmt("pql::parser", "Completed parsing AST: {}", query->toString());
        return query;
    }
}
