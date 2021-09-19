// pql/parser.cpp

#include <unordered_set>

#include <zpr.h>

#include "util.h"
#include "exceptions.h"
#include "simple/parser.h"
#include "pql/parser/parser.h"

namespace pql::parser
{
    using PqlException = util::PqlException;


    struct ParserState
    {
        zst::str_view stream;

        zst::str_view raw_stream() const
        {
            return this->stream;
        }

        Token next()
        {
            return getNextToken(this->stream);
        }

        Token peek_one() const
        {
            return peekNextOneToken(this->stream);
        }

        std::vector<Token> peek_two() const
        {
            return peekNextTwoTokens(this->stream);
        }
    };

    // Clauses
    const Token KW_Select { "Select", TT::Identifier };
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

    // Process the next token as a variable and insert it into declaration_list
    void insert_var_to_declarations(ParserState* ps, ast::DeclarationList* declaration_list, ast::DESIGN_ENT ent)
    {
        Token var = ps->next();

        if(var.type != TT::Identifier)
        {
            throw PqlException(
                "pql::parser", "expected variable name to be an identifier but received {} instead.", var.text);
        }
        if(declaration_list->hasDeclaration(var.text.str()))
            throw util::PqlException("pql::parser", "duplicate declaration '{}'", var.text);

        util::log("pql::parser", "Adding declaration {} to declaration list", var.text);
        declaration_list->addDeclaration(var.text.str(), ent);
    }

    // Process the next tokens as the start of an entity declaration and insert declarations into declaration_list
    void insert_declaration(ParserState* ps, ast::DeclarationList* declaration_list)
    {
        auto check_semicolon = [](ParserState* ps) {
            if(ps->next() != TT::Semicolon)
                throw PqlException("pql::parser", "expected semicolon after statement");
        };
        auto check_comma = [](ParserState* ps) {
            if(ps->next() != TT::Comma)
                throw PqlException("parser", "expected semicolon after statement");
        };

        if(KW_DesignEntities.count(ps->peek_one()) == 0)
        {
            throw PqlException("pql::parser", "Expected declarations to start with design-entity keyword instead of {}",
                ps->peek_one().text);
        }

        std::string ent_string { ps->next().text.str() };
        util::log("pql::parser", "Parsing declaration with design_ent:{}", ent_string);

        if(ast::DESIGN_ENT_MAP.count(ent_string) == 0)
        {
            util::log("pql::parser", "Invalid entity provided in declaration {}", ent_string);
        }
        ast::DESIGN_ENT ent = ast::DESIGN_ENT_MAP.find(ent_string)->second;
        insert_var_to_declarations(ps, declaration_list, ent);

        // Handle trailing additional var using `,`
        while(ps->peek_one().type == TT::Comma)
        {
            check_comma(ps);
            insert_var_to_declarations(ps, declaration_list, ent);
        }
        check_semicolon(ps);
    }


    ast::EntRef parse_ent_ref(ParserState* ps, const ast::DeclarationList* declaration_list)
    {
        Token tok = ps->next();
        if(tok.type == TT::Underscore)
        {
            return ast::EntRef::ofWildcard();
        }
        if(tok.type == TT::DoubleQuotes)
        {
            Token name_declaration_tok = ps->next();
            if(name_declaration_tok.type != TT::Identifier)
            {
                throw PqlException("pql::parser", "Expected named declaration to be an identifier instead of {}",
                    name_declaration_tok.text);
            }
            if(ps->next() != TT::DoubleQuotes)
            {
                throw PqlException("pql::parser", "Expected named declaration to end with double quotes(\")");
            }

            return ast::EntRef::ofName(name_declaration_tok.text.str());
        }
        if(tok.type == TokenType::Identifier)
        {
            std::string var_name = tok.text.str();

            auto declaration = declaration_list->getDeclaration(var_name);
            if(declaration == nullptr)
                throw PqlException("pql::parser", "Undeclared entity {} provided when parsing ent ref", var_name);

            return ast::EntRef::ofDeclaration(declaration);
        }
        throw PqlException("pql::parser", "Invalid ent ref starting with {}", tok.text);
    }

    ast::StmtRef parse_stmt_ref(ParserState* ps, const ast::DeclarationList* declaration_list)
    {
        Token tok = ps->next();
        if(tok.type == TT::Underscore)
        {
            return ast::StmtRef::ofWildcard();
        }
        if(tok.type == TT::Number)
        {
            size_t id = 0;
            for(char c : tok.text)
                id = 10 * id + (c - '0');

            return ast::StmtRef::ofStatementId(id);
        }
        if(tok.type == TokenType::Identifier)
        {
            std::string var_name = tok.text.str();

            auto declaration = declaration_list->getDeclaration(var_name);
            if(declaration == nullptr)
                throw PqlException("pql::parser", "Undeclared entity {} provided when parsing stmt ref", var_name);

            return ast::StmtRef::ofDeclaration(declaration);
            ;
        }
        throw PqlException("pql::parser", "Invalid stmt ref starting with {}", tok.text);
    }

    // Extract out the expression between the double quotes in expression specification.
    std::unique_ptr<simple::ast::Expr> parse_expr(ParserState* ps)
    {
        if(ps->next().type != TT::DoubleQuotes)
        {
            throw PqlException("pql::parser", "Expect expression to start with double quotes(\")");
        }

        zst::str_view expr_str = extractTillQuotes(ps->stream);

        if(ps->next().type != TT::DoubleQuotes)
        {
            throw PqlException("pql::parser", "Expect expression to  with double quotes(\")");
        }

        return simple::parser::parseExpression(expr_str);
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

        if(ps->peek_one() == TT::DoubleQuotes)
            expr_spec.expr = parse_expr(ps);

        // '_' itself is valid as well, so don't expect '__'
        if(is_subexpr && expr_spec.expr != nullptr)
        {
            if(ps->next() != TT::Underscore)
                throw PqlException("pql::parser", "expected '_' in subexpression pattern");
        }

        expr_spec.is_subexpr = is_subexpr;
        return expr_spec;
    }

    ast::PatternCl parse_pattern(ParserState* ps, const ast::DeclarationList* declaration_list)
    {
        std::vector<std::unique_ptr<ast::PatternCond>> pattern_conds;

        if(Token tok = ps->next(); tok != KW_Pattern)
        {
            throw PqlException(
                "pql::parser", "Pattern clauses needs to start with 'pattern' keyword instead of {}", tok.text);
        }

        Token declaration_tok = ps->next();

        auto declaration = declaration_list->getDeclaration(declaration_tok.text.str());
        if(declaration == nullptr)
        {
            throw PqlException("pql::parser",
                "Pattern condition should start with the synonym of a previous declaration instead of {}",
                declaration_tok.text);
        }

        if(declaration->design_ent != ast::DESIGN_ENT::ASSIGN)
        {
            throw PqlException("pql::parser", "Expected assign declaration instead of {}",
                ast::INV_DESIGN_ENT_MAP.find(declaration->design_ent)->second);
        }

        util::log("pql::parser", "Parsing pattern clause with assignment condition {}", declaration->toString());

        // TOOD refactor to a smaller method.
        auto pattern_cond = std::make_unique<ast::AssignPatternCond>();


        if(Token tok = ps->next(); tok != TT::LParen)
        {
            throw PqlException("pql::parser", "Expected '(' after synonym in Pattern clause instead of {}", tok.text);
        }

        pattern_cond->assignment_declaration = declaration;
        pattern_cond->ent = parse_ent_ref(ps, declaration_list);

        if(Token tok = ps->next(); tok != TT::Comma)
        {
            throw PqlException("pql::parser", "Expected ',' after ent ref declaration instead of {}", tok.text);
        }
        pattern_cond->expr_spec = parse_expr_spec(ps);
        if(Token tok = ps->next(); tok != TT::RParen)
        {
            throw PqlException("pql::parser", "Expected ')' after expr spec in Pattern clause instead of {}", tok.text);
        }

        util::log("pql::parser", "Completed parsing pattern cond: {}", pattern_cond->toString());
        pattern_conds.push_back(std::move(pattern_cond));

        return ast::PatternCl { std::move(pattern_conds) };
    }

    std::unique_ptr<ast::FollowsT> parse_follows_t(ParserState* ps, const ast::DeclarationList* declaration_list)
    {
        auto f = ps->next();
        auto star = ps->next();
        auto f_star = zst::str_view(f.text.data(), strlen("Follows*"));

        if(f.text != "Follows" || star != TT::Asterisk || f_star != "Follows*")
        {
            throw PqlException("pql::parser", "FollowsT relationship condition should start with 'Follows*'");
        }

        auto follows_t = std::make_unique<ast::FollowsT>();
        if(Token tok = ps->next(); tok != TT::LParen)
        {
            throw PqlException("pql::parser", "Expected '(' at the start of 'Follows*' instead of {}", tok.text);
        }
        follows_t->before = parse_stmt_ref(ps, declaration_list);
        if(Token tok = ps->next(); tok != TT::Comma)
        {
            throw PqlException("pql::parser", "Expected ',' after declaring ent ref in Follows*");
        }
        follows_t->after = parse_stmt_ref(ps, declaration_list);
        if(Token tok = ps->next(); tok != TT::RParen)
        {
            throw PqlException("pql::parser", "Expected ')' at the end of 'Follows*' instead of {}", tok.text);
        }

        return follows_t;
    }

    std::unique_ptr<ast::Follows> parse_follows(ParserState* ps, const ast::DeclarationList* declaration_list)
    {
        if(ps->next().text != "Follows")
        {
            throw PqlException("pql::parser", "FollowsT relationship condition should start with 'Follows*'");
        }
        auto follows = std::make_unique<ast::Follows>();
        if(Token tok = ps->next(); tok != TT::LParen)
        {
            throw PqlException("pql::parser", "Expected '(' at the start of 'Follows' instead of {}", tok.text);
        }
        follows->directly_before = parse_stmt_ref(ps, declaration_list);
        if(Token tok = ps->next(); tok != TT::Comma)
        {
            throw PqlException("pql::parser", "Expected ',' after declaring ent ref in Follows");
        }
        follows->directly_after = parse_stmt_ref(ps, declaration_list);
        if(Token tok = ps->next(); tok != TT::RParen)
        {
            throw PqlException("pql::parser", "Expected ')' at the end of 'Follows' instead of {}", tok.text);
        }
        util::log("pql::parser", "Parsed: {}", follows->toString());

        return follows;
    }

    std::unique_ptr<ast::ParentT> parse_parent_t(ParserState* ps, const ast::DeclarationList* declaration_list)
    {
        auto p = ps->next();
        auto star = ps->next();
        auto p_star = zst::str_view(p.text.data(), strlen("Parent*"));

        if(p.text != "Parent" || star != TT::Asterisk || p_star != "Parent*")
        {
            throw PqlException("pql::parser", "ParentT relationship condition should start with 'Parent*'");
        }
        auto parent_t = std::make_unique<ast::ParentT>();
        if(Token tok = ps->next(); tok != TT::LParen)
        {
            throw PqlException("pql::parser", "Expected '(' at the start of 'Parent*' instead of {}", tok.text);
        }
        parent_t->ancestor = parse_stmt_ref(ps, declaration_list);
        if(Token tok = ps->next(); tok != TT::Comma)
        {
            throw PqlException("pql::parser", "Expected ',' after declaring ent ref in Parent*");
        }
        parent_t->descendant = parse_stmt_ref(ps, declaration_list);
        if(Token tok = ps->next(); tok != TT::RParen)
        {
            throw PqlException("pql::parser", "Expected ')' at the end of 'Parent*' instead of {}", tok.text);
        }

        return parent_t;
    }

    std::unique_ptr<ast::Parent> parse_parent(ParserState* ps, const ast::DeclarationList* declaration_list)
    {
        if(ps->next().text != "Parent")
        {
            throw PqlException("pql::parser", "Parent relationship condition should start with 'Parent'");
        }
        auto parent = std::make_unique<ast::Parent>();
        if(Token tok = ps->next(); tok != TT::LParen)
        {
            throw PqlException("pql::parser", "Expected '(' at the start of 'Parent' instead of {}", tok.text);
        }
        parent->parent = parse_stmt_ref(ps, declaration_list);
        if(Token tok = ps->next(); tok != TT::Comma)
        {
            throw PqlException("pql::parser", "Expected ',' after declaring ent ref in Parent");
        }
        parent->child = parse_stmt_ref(ps, declaration_list);
        if(Token tok = ps->next(); tok != TT::RParen)
        {
            throw PqlException("pql::parser", "Expected ')' at the end of 'Parent' instead of {}", tok.text);
        }

        return parent;
    }

    bool is_next_stmt_ref(ParserState* ps, const ast::DeclarationList* declaration_list)
    {
        Token tok = ps->peek_one();
        if(tok.type == TT::Number)
        {
            // Only statement ref can be number
            return true;
        }

        if(tok.type == TT::Underscore)
        {
            // All entity refs are statements
            return true;
        }
        if(tok.type == TT::DoubleQuotes)
        {
            // Only entity refs are surrounded by double quotes.
            return false;
        }

        // Only ref to previously declared entity allowed
        if(tok.type != TT::Identifier)
        {
            throw PqlException("pql::parser",
                "StmtRef,EntRef should start with number, underscore, '\"' or identifier instead of {}", tok.text);
        }

        auto decl = declaration_list->getDeclaration(tok.text.str());
        if(decl == nullptr)
            throw PqlException("pql::parser", "{} was not previously declared", tok.text);

        // Check if the reference to previously declared entity is a stmt.
        return ast::kStmtDesignEntities.count(decl->design_ent) > 0;
    }

    std::unique_ptr<ast::Uses> parse_uses(ParserState* ps, const ast::DeclarationList* declaration_list)
    {
        if(ps->next().text != "Uses")
        {
            throw PqlException("pql::parser", "Uses relationship condition should start with 'Uses'");
        }

        if(Token tok = ps->next(); tok.type != TT::LParen)
        {
            throw PqlException("pql::parser", "Expected '(' at the start of 'Uses' instead of {}", tok.text);
        }
        bool is_user_stmt_ref = is_next_stmt_ref(ps, declaration_list);

        if(is_user_stmt_ref)
        {
            auto user = parse_stmt_ref(ps, declaration_list);
            if(Token tok = ps->next(); tok != TT::Comma)
            {
                throw PqlException("pql::parser", "Expected ',' after declaring stmt ref in Uses");
            }
            auto ent = parse_ent_ref(ps, declaration_list);
            if(Token tok = ps->next(); tok.type != TT::RParen)
            {
                throw PqlException("pql::parser", "Expected ')' at the end of 'Uses' instead of {}", tok.text);
            }
            auto user_s = std::make_unique<ast::UsesS>();
            user_s->user = user;
            user_s->ent = ent;
            return user_s;
        }
        else
        {
            auto user = parse_ent_ref(ps, declaration_list);
            if(Token tok = ps->next(); tok != TT::Comma)
            {
                throw PqlException("pql::parser", "Expected ',' after declaring ent ref in Uses");
            }
            auto ent = parse_ent_ref(ps, declaration_list);
            if(Token tok = ps->next(); tok.type != TT::RParen)
            {
                throw PqlException("pql::parser", "Expected ')' at the end of 'Uses' instead of {}", tok.text);
            }
            auto user_p = std::make_unique<ast::UsesP>();
            user_p->user = user;
            user_p->ent = ent;
            return user_p;
        }
    }

    std::unique_ptr<ast::Modifies> parse_modifies(ParserState* ps, const ast::DeclarationList* declaration_list)
    {
        if(ps->next().text != "Modifies")
        {
            throw PqlException("pql::parser", "Modifies relationship condition should start with 'Modifies'");
        }

        if(Token tok = ps->next(); tok.type != TT::LParen)
        {
            throw PqlException("pql::parser", "Expected '(' at the start of 'Modifies' instead of {}", tok.text);
        }
        bool is_modifier_stmt_ref = is_next_stmt_ref(ps, declaration_list);

        if(is_modifier_stmt_ref)
        {
            auto modifier = parse_stmt_ref(ps, declaration_list);
            if(Token tok = ps->next(); tok != TT::Comma)
            {
                throw PqlException("pql::parser", "Expected ',' after declaring stmt ref in Modifies");
            }
            auto ent = parse_ent_ref(ps, declaration_list);
            if(Token tok = ps->next(); tok.type != TT::RParen)
            {
                throw PqlException("pql::parser", "Expected ')' at the end of 'Modifies' instead of {}", tok.text);
            }
            auto modifies_s = std::make_unique<ast::ModifiesS>();
            modifies_s->modifier = modifier;
            modifies_s->ent = ent;
            return modifies_s;
        }
        else
        {
            auto modifier = parse_ent_ref(ps, declaration_list);
            if(Token tok = ps->next(); tok != TT::Comma)
            {
                throw PqlException("pql::parser", "Expected ',' after declaring stmt ref in Modifies");
            }
            auto ent = parse_ent_ref(ps, declaration_list);
            if(Token tok = ps->next(); tok.type != TT::RParen)
            {
                throw PqlException("pql::parser", "Expected ')' at the end of 'Modifies' instead of {}", tok.text);
            }
            auto modifies_s = std::make_unique<ast::ModifiesP>();
            modifies_s->modifier = modifier;
            modifies_s->ent = ent;
            return modifies_s;
        }
    }

    std::unique_ptr<ast::RelCond> parse_rel_cond(ParserState* ps, const ast::DeclarationList* declaration_list)
    {
        std::vector<Token> rel_cond_toks = ps->peek_two();

        // Check relationship with 2 tokens first
        if(rel_cond_toks == KW_FollowsT)
            return parse_follows_t(ps, declaration_list);
        if(rel_cond_toks == KW_ParentT)
            return parse_parent_t(ps, declaration_list);

        if(rel_cond_toks[0] == KW_Follows)
            return parse_follows(ps, declaration_list);
        if(rel_cond_toks[0] == KW_Parent)
            return parse_parent(ps, declaration_list);
        if(rel_cond_toks[0] == KW_Uses)
            return parse_uses(ps, declaration_list);
        if(rel_cond_toks[0] == KW_Modifies)
            return parse_modifies(ps, declaration_list);
        throw PqlException("pql::parser", "Invalid relationship condition tokens: {}, {}", rel_cond_toks[0].text,
            rel_cond_toks[1].text);
    }

    ast::SuchThatCl parse_such_that(ParserState* ps, const ast::DeclarationList* declaration_list)
    {
        auto such = ps->next();
        auto that = ps->next();

        // there must be exactly a space between 'such' and 'that'. check this by "extending" the
        // length of the 'such' token, which we know is safe.
        auto tmp_token = zst::str_view(such.text.data(), strlen("such that"));

        if(such.text != "such" || that.text != "that" || tmp_token != "such that")
            throw PqlException("pql::parser", "Such That clause should start with 'such that'");

        util::log("pql::parser", "Parsing such that clause. Remaining {}", ps->stream);
        ast::SuchThatCl such_that {};

        // TODO(iteration 2): Handle AND condition here
        such_that.rel_conds.push_back(parse_rel_cond(ps, declaration_list));

        util::log("pql::parser", "Complete parsing such that clause: {}", such_that.toString());
        return such_that;
    }

    ast::Select parse_select(ParserState* ps, const ast::DeclarationList* declaration_list)
    {
        Token select_tok = ps->next();
        if(select_tok != KW_Select)
        {
            throw PqlException(
                "pql::parser", "Select clauses should start with `Select` instead of {}", select_tok.text);
        }

        Token var_tok = ps->next();
        if(var_tok.type != TT::Identifier)
        {
            throw PqlException(
                "pql::parser", "Expected synonym at the start of Select clause instead of {}", var_tok.text);
        }

        auto ent = declaration_list->getDeclaration(var_tok.text.str());
        if(ent == nullptr)
            throw PqlException("pql::parser", "Undeclared {} entity provided.", var_tok.text.str());

        util::log("pql::parser", "Return ent for Select clause: {}", ent->toString());

        ast::Select select {};
        select.ent = ent;

        std::vector<Token> clause_tok = ps->peek_two();
        bool allow_pattern = true;
        bool allow_such_that = true;

        // TOOD(#100): Remove single pattern or single such that clause after iteration 1.
        while(((clause_tok[0] == KW_Pattern) && allow_pattern) || ((clause_tok == KW_SuchThat) && allow_such_that))
        {
            if(clause_tok[0] == KW_Pattern)
            {
                util::log("pql::parser", "Parsing pattern clause");
                select.pattern = parse_pattern(ps, declaration_list);
                allow_pattern = false;
            }
            else if(clause_tok == KW_SuchThat)
            {
                util::log("pql::parser", "Parsing such that clause");
                select.such_that = parse_such_that(ps, declaration_list);
                allow_such_that = false;
            }
            clause_tok = ps->peek_two();
        }
        util::log("pql::parser", "Completed parsing Select clause :{}", select.toString());

        return select;
    }

    std::unique_ptr<ast::Query> parsePQL(zst::str_view input)
    {
        util::log("pql::parer", "Parsing input {}", input);
        auto ps = ParserState { input };
        auto query = std::make_unique<ast::Query>();

        bool found_select = false;
        for(Token t; (t = ps.peek_one()) != TT::EndOfFile;)
        {
            if(t == KW_Select)
            {
                util::log("pql::parser", "parsing Select");
                query->select = parse_select(&ps, &query->declarations);

                found_select = true;
                if(ps.peek_one() != TT::EndOfFile)
                {
                    throw util::PqlException(
                        "pql::parser", "Query should end after a single select clause instead of {}", ps.stream);
                }
            }
            else
            {
                util::log("pql::parser", "parsing declaration");
                insert_declaration(&ps, &query->declarations);
            }
        }

        if(!found_select)
            throw util::PqlException("pql::parser", "All queries should contain a select clause");

        util::log("pql::parser", "Completed parsing AST: {}", query->toString());
        return query;
    }
}
