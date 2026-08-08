-- Lược đồ cơ sở dữ liệu SQLite cho Cynara Policy DB
CREATE TABLE policies (
    client_label TEXT,
    user TEXT,
    privilege TEXT,
    result INTEGER, -- 1 for ALLOW, 0 for DENY
    PRIMARY KEY (client_label, user, privilege)
);

-- Dữ liệu mẫu khởi tạo
INSERT INTO policies VALUES ('System', '*', 'http://tizen.org/privilege/internet', 1);
