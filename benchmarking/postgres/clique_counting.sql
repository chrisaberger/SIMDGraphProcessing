\timing on
SELECT COUNT(*) FROM
   edge e1, edge e2, edge e3, edge e4, edge e5, edge e6
WHERE
   e1.b = e2.a AND e2.b = e3.a AND e3.b = e4.b AND e1.a = e4.a AND e5.a = e1.b AND e5.b = e3.b AND e6.a = e1.a AND e6.b = e3.a;
\timing off
