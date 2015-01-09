from sets import Set

def build_tree(U, es):
    if all((e_i & U) == set() for e_i in es):
        return None
    else:
        lc, rc = None, None
        if len(es) > 1 and any(not (U <= e_i) for e_i in es):
            e_k = es[-1]
            lc = build_tree(U - e_k, es[0: -1])
            rc = build_tree(U & e_k, es[0: -1])
        return len(es), U, lc, rc

def print_tree(t, level = 0, prefix = "r"):
    if t != None:
        label, univ, lc, rc = t
        print (" " * level) + prefix + " " + str(label) + " " + str(univ)
        print_tree(lc, level + 1, "l")
        print_tree(rc, level + 1, "r")

if __name__ == "__main__":
    tc = build_tree(set(["x", "y", "z"]), [set(["x", "y"]), set(["y", "z"]), set(["x", "z"])])
    print "Triangle listing"
    print_tree(tc)

    fig1 = build_tree(set(["A_1", "A_2", "A_3", "A_4", "A_5", "A_6"]), [set(["A_1", "A_2", "A_4", "A_5"]), set(["A_1", "A_3", "A_4", "A_6"]), set(["A_1", "A_2", "A_3"]), set(["A_2", "A_4", "A_6"]), set(["A_3", "A_5", "A_6"])])
    print "Example in Figure 1 in Worst-Case Join paper"
    print_tree(fig1)
