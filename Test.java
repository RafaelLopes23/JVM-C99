public class Test {
    public static void main(String[] args) {
        // Test ICONST_* (0x03-0x08)
        int a = 5;      // ICONST_5
        int b = 3;      // ICONST_3
        int c = 1;      // ICONST_1
        int d = 0;      // ICONST_0
        int e = 2;      // ICONST_2
        int f = 4;      // ICONST_4

        // Test BIPUSH and SIPUSH (0x10, 0x11)
        int g = 100;    // BIPUSH 100
        int h = 1000;   // SIPUSH 1000

        // Test ILOAD and ISTORE (0x15, 0x36)
        int i = a;      // ILOAD + ISTORE

        // Test Stack Operations (0x57, 0x59)
        int j = i;      // DUP + ISTORE
        i = 0;         // POP (previous value) + ICONST_0 + ISTORE

        // Test Math Operations (0x60, 0x64, 0x68, 0x6C, 0x80)
        int sum = a + b;    // IADD
        int diff = a - b;   // ISUB
        int prod = a * b;   // IMUL
        int quot = a / b;   // IDIV
        int or = a | b;     // IOR

        // Print results
        System.out.println("a: " + a);
        System.out.println("b: " + b);
        System.out.println("c: " + c);
        System.out.println("d: " + d);
        System.out.println("e: " + e);
        System.out.println("f: " + f);
        System.out.println("g: " + g);
        System.out.println("h: " + h);
        System.out.println("i: " + i);
        System.out.println("j: " + j);
        System.out.println("sum: " + sum);
        System.out.println("diff: " + diff);
        System.out.println("prod: " + prod);
        System.out.println("quot: " + quot);
        System.out.println("or: " + or);
    }
}
