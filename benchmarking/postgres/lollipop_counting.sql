\timing on
SELECT COUNT(*) FROM
  uedge e1, uedge e2, uedge e3, uedge e4
WHERE
  e1.b = e2.a AND e2.b = e3.b AND e1.a = e3.a AND e1.a = e4.a AND
  e2.a < e2.b;
\timing off
