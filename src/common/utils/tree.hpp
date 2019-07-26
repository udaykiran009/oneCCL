#pragma once

class bin_tree
{
public:

    bin_tree(size_t comm_size,
             size_t rank,
             bool is_main = true) : comm_size(comm_size), rank(rank), is_main(is_main)
    {
        calc_height(is_main);

        if (rank == default_root)
        {
            p = -1;
            l = -1;
            if (is_main)
            {
                r = height > 0 ? 1 << (height - 1) : -1;
            }
            else
            {
                if (comm_size == 1u << height)
                {
                    r = height > 0 ? (1 << height) - 1 : -1;
                }
                else
                {
                    r = height > 0 ? (1 << (height - 1)) - 1 : -1;
                }
            }
            return;
        }

        calc_parent();
        if (height > 0)
        {
            calc_left();
            calc_right();
        }
    }

    bin_tree(const bin_tree& other) = default;
    bin_tree& operator=(const bin_tree& other) = default;

    ssize_t left() const
    {
        return l;
    }

    ssize_t right() const
    {
        return r;
    }

    ssize_t parent() const
    {
        return p;
    }

    bin_tree copy_with_new_root(size_t new_root) const
    {
        bin_tree copy(*this);
        ssize_t root = static_cast<ssize_t>(new_root);

        //if current node will become a new root or node was a default root - the tree must be reconstruced
        if (copy.rank == root || copy.rank == default_root)
        {
            //create part of tree with the default root
            copy = bin_tree(static_cast<size_t>(comm_size), copy.rank == default_root ? root : default_root, is_main);
            copy.rank = root;
        }

        //swap default root with new root in any of left/right/parent nodes
        copy.reset_connections(root);

        return copy;
    }

    friend std::ostream& operator<<(std::ostream& str,
                                    const bin_tree& tree)
    {
        str << "parent " << tree.p <<
            " -> rank " << tree.rank <<
            " -> [left " << tree.l <<
            ", right " << tree.r << "]";
        return str;
    }

private:

    void reset_connections(ssize_t new_root)
    {
        swap_if_any_of(p, default_root, new_root);
        swap_if_any_of(l, default_root, new_root);
        swap_if_any_of(r, default_root, new_root);
    }

    static void swap_if_any_of(ssize_t& node,
                        ssize_t val1,
                        ssize_t val2)
    {
        if (node == val1)
        {
            node = val2;
        }
        else if (node == val2)
        {
            node = val1;
        }
    }

    void calc_height(bool main_tree)
    {
        if (main_tree || rank == default_root)
        {
            while ((rank & (1u << height)) == 0 && (1u << height) < comm_size)
            {
                ++height;
            }
        }
        else
        {
            while ((rank & (1u << height)) != 0 && (1u << height) < comm_size)
            {
                ++height;
            }
        }
    }

    void calc_parent()
    {
        //find a parent using height, assume that rank is a right child
        ssize_t possible_parent_as_left = rank + (1 << height);
        //right child has a bit `1` a the position `height + 1` due to it is calculated as `parent + 2^(heightP-1)`
        //where heightP is parent's height i.e height + 1

        if ((rank & (1 << (height + 1))) ||
            //parent of the left rank is always bigger than its parent, check that we do not exceed comm size
            possible_parent_as_left > comm_size - 1)
        {
            //this is right child
            p = rank - (1 << height);
            if (p < 0)
            {
                p = 0;
            }
        }
        else
        {
            p = possible_parent_as_left;
        }
    }

    void calc_left()
    {
        l = rank - (1 << (height - 1));
        if (l <= 0)
        {
            l = -1;
        }
    }

    void calc_right()
    {
        r = rank + (1 << (height - 1));
        ssize_t limit = comm_size - 1;

        if (r > limit)
        {
            auto height_tmp = height;
            //need to decrease height to find most suitable right -- topmost right leaf case
            do
            {
                --height_tmp;
                if (height_tmp == 0)
                {
                    r = -1;
                    break;
                }
                r = rank + (1 << (height_tmp - 1));

            } while (r > limit);
        }
    }

    ssize_t comm_size;
    ssize_t rank;
    ssize_t height = 0;
    ssize_t p = -1;
    ssize_t l = -1;
    ssize_t r = -1;
    bool is_main;

    static const ssize_t default_root = 0;
};

class double_tree
{
public:
    double_tree(size_t comm_size,
                size_t rank) :
        t1(comm_size, rank, true),
        t2(comm_size, rank, false)
    {
        LOG_DEBUG("T1: ", t1);
        LOG_DEBUG("T2: ", t2);
    }

    /**
     * Binary tree which consists of the current rank as a node and possible parent, left and right children.
     * Even ranks numbers are always inner nodes, odd rank numbers are always leaves
     * @return binary tree t1
     */
    const bin_tree& T1() const
    {
        return t1;
    }

    /**
     * Binary tree which consists of the current rank as a node and possible parent, left and right children.
     * Even ranks numbers are always leaves, odd rank numbers are always inner nodes
     * @return binary tree t2
     */
    const bin_tree& T2() const
    {
        return t2;
    }

    double_tree copy_with_new_root(size_t new_root) const
    {
        return double_tree(t1.copy_with_new_root(new_root), t2.copy_with_new_root(new_root));
    }

private:

    double_tree(bin_tree t1,
                bin_tree t2) : t1(t1), t2(t2)
    {
    }

    bin_tree t1;
    bin_tree t2;
};
