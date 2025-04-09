class TestProgram {
    int x;
    float y;
    
    public static void main(String[] args) {
        int a = 10;
        int b = 20;
        int result = a + b;
        
        if (result > 25) {
            System.out.println("Result is greater than 25");
        } else {
            System.out.println("Result is not greater than 25");
        }
        
        for (int i = 0; i < 5; i++) {
            System.out.println("Loop iteration");
        }
        
        try {
            int division = a / (b - 20);  // Division by zero
        } catch (Exception e) {
            System.out.println("Exception caught");
        }
    }
    
    int calculate(int x, int y) {
        return x * y + 5;
    }
}