Qué se guarda en el fichero comprimido.huff
<pre>
Orden	   Dato             	          Tamaño	      Propósito
1	       longitudOriginal	            4 bytes	      Saber cuántos caracteres decodificar
2	       Árbol serializado(preorden)	variable	    Reconstruir exactamente el mismo árbol
3        Bits empaquetados	          variable	    El texto comprimido real</pre>
