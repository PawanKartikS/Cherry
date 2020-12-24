# Cherry
Cherry is an interpreter for a high-level functional programming language. It is written in pure C and is designed to -
1. Have an extremely low overhead, allowing fast execution of programs.
2. Well defined memory management, allowing it to quit gracefully without leaving any memory leaks behind.

#### Implemented -
1. Console IO
2. If-else, looping
3. String slicing
4. Functions - user defined & built-in (with vargs!)
5. Ability to defer functions (like `Golang`!)
6. Recursion
7. Detection of unreachable or dead code and conditions (`if` and `for`) that always evaluate to true or false
8. Detection of possible infinite loops
9. Variable scoping
10. Generic containers (as of now `List<T>` & `Stack<T>`).
11. Optimization techniques (as of now constant folding)

#### In progress -
1. Functions for generic containers like `add()`, `get()`, `size()` and so on.
2. Expansion of built-in functions
3. Further improvements to memory management

#### Compilations -
```
cmake . && make
./cherry <sourcefile>
```

#### Example
```py
# Example for Cherry

# Helper function that is called by foo()
def helper()
    return 'return::foo()'
end

def foo()
    print 'calling helper()'

    # Return helper()'s result
    return helper()
end

def pi_lhs()
    return 3.14
end

def pi_rhs(x)
    return x
end

# Recursively count from num to 0
def rec_count(num)
    if num <= 0
        return
    end

    print num
    num--
    rec_count(num)
end

# Main function.
# Entry point for the interpreter
def main()

    print 'calling foo()'
    print foo()

    # If statement
    if pi_lhs() == pi_rhs(+3.14)
        print 'pi_lhs() == pi_rhs()'
    end

    if pi_lhs() == +3.14
        print 'pi_lhs() == +3.14'
    end

    if 3.14 == pi_rhs(+3.14)
        print '3.14 == pi_rhs()'
    end

    # Count from 10 to 0
    rec_count(10)

    # Call builtin string function to reverse the string (arg)
    var revstr = rev("pawan")
    put("The reverse of pawan is", revstr)

    # For loop to count from 10 to 0
    var i = 10
    for i >= 0
        print i
        i--
    end
    
    # String slicing
    var name = 'pawan'
    var i = 0
    
    for i <= 5
        print name[i:i+3]
        i++
    end
end
```
