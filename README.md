# ControlMechanisms-ScalableComputing

Após descompactar a pasta com os arquivos, siga os passos abaixo para executar o programa.

- Compile o arquivo controlling.cpp com o comando:
```g++ controlling.cpp -o3 controlling.exe```

- Execute o arquivo controlling.cpp com 
```./controlling.cpp ```

- Para evitar flood do terminal, limitamos o número de primos que é printado durante a execução. Você pode ajustar isso mudando o valor de MAX_PRIMES em controlling.cpp

- Caso você queira executar cada variação de número de threads mais vezes para obter um resultado mais preciso, altere o valor de MAX_ITERATIONS em controlling.cpp (default = 45). Caso faça isso, mude N em convert_data.ipynb para o mesmo valor.

- Caso deseje manipular os dados, use o notebook convert_data para converter-los executando o primeiro bloco do notebook. O segundo bloco calcula as médias de cada variação de número de threads e printa a diferença entre os valores de balanced e unbalanced.

- Para gerar os gráficos, execute o terceiro bloco do notebook convert_data.ipynb.

- O formato dos dados após usar o notebook é um csv onde as colunas são os números de threads e as linhas são o tempo de execução.