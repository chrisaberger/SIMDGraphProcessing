DROP TABLE IF EXISTS edge;
CREATE TABLE edge(a BIGINT, b BIGINT);

COPY edge FROM :'dataset';

CREATE INDEX edge_a_index ON edge(a);
CREATE INDEX edge_b_index ON edge(b);
