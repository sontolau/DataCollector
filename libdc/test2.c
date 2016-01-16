#define run(x) x

void main ()
{
   run ({
       if (1)
         printf ("hello\n");
       printf ("%d=%d", 10, 11);
 })
}
