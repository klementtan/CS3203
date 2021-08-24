# 24 Aug Meeting Notes

## Discussion

* PR for basic parser(#1).
* PKB will expose method for PQL to evaluate queries.
* PQL will separate into query parser and query evaluater.
* Delegaton of project:
  * Parser sub team:
    * Parse program to AST.
    * Work on test cases.
  * PKB sub team:
    * Getting AST from parser.
    * Exposing methods to evaluate queries of relationships for PQL.
  * PQL sub team:
    * Parse query from user.
    * Call evaluate query method from PKB.

## Decisions

* Style:
  * All public method should be `camelCase` and `snake_case` for internal method/file-only methods.
* Will have Sat night meetings if we have something to discuss.

## Actionables

* Have some code to show by next consult.
