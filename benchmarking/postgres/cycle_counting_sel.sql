\timing on
SELECT COUNT(*) FROM
   edge e1, edge e2, edge e3, edge e4,
   node n1, node n2, node n3, node n4
WHERE
   e1.year = 2012 AND e2.year = 2012 AND e3.year = 2012 AND e4.year = 2012 AND
   n1.place > 500 AND n2.place > 500 AND n3.place > 500 AND n4.place > 500 AND
   n1.a = e1.a AND n2.a = e2.a AND n3.a = e3.a AND n4.a = e4.b AND
   e1.b = e2.a AND e2.b = e3.a AND e3.b = e4.b AND e1.a = e4.a;
\timing off
