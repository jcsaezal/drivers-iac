#include <linux/module.h>	/* Requerido por todos los módulos */
#include <linux/kernel.h>	/* Definición de KERN_INFO */
MODULE_LICENSE("GPL"); 	/*  Licencia del modulo */

/* Función que se invoca cuando se carga el módulo en el kernel */
int modulo_hello_init(void)
{
	printk(KERN_INFO "Modulo Hello cargado. Hola kernel.\n");

	/* Devolver 0 para indicar una carga correcta del módulo */
	return 0;
}

/* Función que se invoca cuando se descarga el módulo del kernel */
void modulo_hello_clean(void)
{
	printk(KERN_INFO "Modulo Hello descargado. Adios kernel.\n");
}

/* Declaración de funciones init y exit */
module_init(modulo_hello_init);
module_exit(modulo_hello_clean);

