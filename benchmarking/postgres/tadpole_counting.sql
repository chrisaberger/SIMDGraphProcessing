\timing on
SELECT COUNT(*) FROM
  uedge e1, uedge e2, uedge e3, uedge e4, uedge e5, uedge e6
WHERE
  e1.b = e2.a AND e2.b = e3.a AND e3.b = e4.b AND e1.a = e4.a AND
  e1.a = e5.a AND e5.b = e6.a AND
  e2.a < e2.b AND e1.a != e3.a AND e2.a != e3.b AND e5.b != e1.b AND
  e5.b != e2.b AND e5.b != e4.b AND e6.b != e1.a;
\timing off
