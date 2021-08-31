# 31 Auguest Meeting Notes

## Consultation

* What was completed
  * Parser completed.
  * PKB assigning statement number, VarTable and Follows relationship.
  * PQL ast struct completed.
* Technical discussion:
  * Should have intermediary betwen PQL and PKB.
  * Run time should be in PQL and initialization phase in PKB.
  * Need to add a check for cyclic/recursive calls. Adviced to check in the parser.
  * Should do string matching for pattern instead of forming simple ast in pql and
  doing tree matching. (Advice from Adam)
* Test Cases:
  * Each test case should target one componenet PQL/PKB/parser.
* Report:
  * Focus on design decisions. ie why array instead of set?
  * Should start working on report early.
* The pace is ok but PQL is a bit slow.
* Next consult agenda:
  * PKB algo run through.
  * How PQL handle clauses.

## Team meeting

* Use [Expr](https://github.com/nus-cs3203/21s1-cp-spa-team-34/blob/master/Team34/Code34/src/spa/include/ast.h#L26)
  matching for pattern.
* Use PKB will open an api to query by `Expr`.
* Use nested namespace for `frontend/ast` to allow PQL to query have both
  simeple and pql ast.
