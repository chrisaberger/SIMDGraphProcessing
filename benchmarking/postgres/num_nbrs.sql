\timing on
SELECT e.a, COUNT(*) AS cnt FROM edge e GROUP BY e.a ORDER BY cnt DESC LIMIT 1;
\timing off
