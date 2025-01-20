public class Test {
    public static void main(String[] args) {
        // Test ICONST_* (0x03-0x08)
        int a = 5;      // ICONST_5
        int b = 3;      // ICONST_3
        int c = 1;      // ICONST_1
        int d = 0;      // ICONST_0
        int e = 2;      // ICONST_2
        int f = 4;      // ICONST_4

        // Test Math Operations
        int sum = a + b;    // IADD (should be 8)
        int diff = a - b;   // ISUB (should be 2)
        int prod = a * b;   // IMUL (should be 15)
        int quot = a / b;   // IDIV (should be 1)
        int or = a | b;     // IOR (should be 7)

        // Store final values back to verify
        a = sum;   // Should be 8
        b = diff;  // Should be 2
        c = prod;  // Should be 15
        d = quot;  // Should be 1
        e = or;    // Should be 7
    }
}