public class cat2test {
    public static void main(String[] args) {
        // Test
        double sum = 0; // DCONST_0

        // Test Math Operations 
        sum = sum + 1;    // DADD
    }
}

/*faulty
 0x3c istore 
 0x32 aaload
 main method not found when running 
```
public class cat2test {
    public static void main(String[] args) {
        // Test
        double a = 5;      // 
        double b = 3;      // 
        double sum = 0;

        // Test Math Operations 
        sum = a + b;    // DADD
    }
}
```

 */
