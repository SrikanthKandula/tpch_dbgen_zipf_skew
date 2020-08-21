# A parallel zipf-skewed data generator for TPC-H benchmark

## 1. Summary
We edit the data generator in [TPC-H](http://www.tpc.org/tpch/) to introduce frequency skew in several columns of various tables.

We are inspired by and build on top of [prior work](https://www.microsoft.com/en-us/download/details.aspx?id=52430) in the following ways:
* Support parallel data-generation which is useful to generate data at large scale factors.
	* Ensure that the **same** data will be generated independent of degree of parallelism
* Uncorrelate the columns and more closely approximate zipf than prior work
	* Prior work repeats frequent values in a consecutive sequence of rows resulting in correlation between the key column and the skewed column
	* Prior work also forgets previously emitted values possibly resulting in greater skew than desired by the user

## 2. Build
Open `tpch.sln` in Visual Studio 2019 and build. [Tested on Windows 10.] It should be straightforward to build on other OSes and with other tools.

## 3. Usage
Identical to the datagen from [TPC-H](http://www.tpc.org/tpch/) except for one additional option `-z <f>` where the argument, a float, is the zipfian scale factor.

Examples:
* `.\Debug\dbgen.exe -T s -s 1000 -z 2`
Generates the supplier.tbl for a scale factor of 1000 (i.e., 1TB) with a zipf scale factor of 2

* `.\Debug\dbgen.exe -T o -s 1000 -z 1 -C 100 -S 4`
Generates the 4th of 100 chunks with a zipf scale factor of 1 of the orders and lineitem tables

* Using multiple tasks to generate different portions of a table:
```
    for ((i=1; i <= 2; i++)); 
	do 
		mkdir -p task_${i};
		cd task_${i};
		cp ../dists.dss . # tasks need a local copy
		../Debug/dbgen.exe -T o -s 1 -z 1 -C 10 -S $i &
		cd ..;
	done;
```	
With unix-style syntax (cygwin), shows how to run two tasks in parallel in two separate folders. We recommend running tasks in different folders to keep their outputs and inputs apart. The dbgen outputs orders.tbl.${i} and lineitem.tbl.${i} files into each task_${i} folder. Examining the `zipf_debug.log` files in each folder should show that both tasks generate identical manifest and that the second task rolls over its seeds to values that the first task ends with. This is crucial to ensure that outputs are identical irrespective of degree-of-parallelism. Examples of what you might see are present in the `sample_task_${i}` folders in the repo.

## 4. Performance
Please pick a degree-of-parallelism such that each task has a reasonable amount of work (e.g., ~100MB of data to generate or more). The default TPC-H datagen precomputes random strings which can take a while; thus the constant overhead per task is sizable. Introduction of skew does not effect data generation throughput in any substantial way.

## 5. Method description and how to vary some internal parameters (not necessary reading)

Recall that the TPC-H datagen generates uniformly distributed values; that is, the values of nonkey columns
are generated uniformly at random from a pre-specified range. Linkages between tables are also similarly distributed; that is, the numbers of lines in LINEITEM that have a given ORDERKEY is chosen uniformly at random. The main goal of this change is to add skew, specifically zipfian skew, to the extent possible.

Recall that Zipfian skew is such that the value at rank $k$ occurs with frequency proportional to $1/k^\alpha$ where $\alpha$ is the zipfian skew factor. If $\alpha$ is 2, then the frequencies at ranks 1, 2, ... are proportional to 1, 0.25, 0.111, 0.0625, ...  

There are different ways to obtain zipf distributed values.  We seek a method that allows for // execution. Specifically, even when many tasks generate data in parallel, their output should match the output generated by a single task.

We use the following method:
1) Each task pre-computes (in a pseudo-random way) the values at the top N ranks. (In code, this is a constant NumTopRanksPerStream with a default value of 1000.) The tasks also compute the probability with which to pick each of these ranks as well as the residual probability.

2) To pick a zipf distributed random value, each task tosses a biased coin to pick one of the values at the top N ranks or, with the residual probability, uniformly at random from a value that is not in the top N ranks.  The task tries at most 5 times to look for a value that is not in the top N.

3) We edit the ADVANCE_SEED function so that each task rolls the state of its seeds ahead (speed_seed.c) by the number of rows that would be generated by the tasks preceding it. We also edit the "boundary" values used by the row_stop function so that each seed advances by a fixed amount per row regardless of the number of random calls that were made when generating that row.

4) Note that the datagen uses different seeds; roughly one per column. However, some seeds are used to generate values from different ranges (nLow and nHigh). This interferes with step 1 above because picking the pre-computed values requires a fixed range. We use two different strategies to account for this concern. (a) We add more seeds one per RANDOM call. We do this for all the comment column; where the offset and length are picked using different seeds. (b) We only use zipf distribution for some of the RANDOM calls but not all; such as in gen_phone where the last four digits are generated uniformly at random. We do this to reduce the number of seeds without adversely affecting the skew of the column.


Note an important caveat: the above process does not generate proper zipf distribution. In particular, the values corresponding to "residual probability" are generated uniformly at random in the range and then checked to not match with the values that have already been picked to represent the top N ranks. Also, if five consecutive attempts to find a new value fail, we just use the fifth chosen value.  The reason to do this is: we want to make no more than a small number (for efficiency) and a bounded number (to advance the seeds correctly) of random calls.  We analyze the properties of this method next.

* Case of number of distinct values in the range or number of tuples being smaller than N: Many columns in TPC-H have only hundreds of distinct values.  For such columns, the method above generates proper zipfian distribution.

* Dependence on the zipf scale factor: The larger the scale factor the faster the probability decays. Thus, closely approximating a zipfian distribution requires maintaining more ranks when the zipf scale factor is small. The table below shows, for different zipf scale factors, the fraction of the total probability that is contributed by the top N ranks:


Zipf Scale Factor | N= 100 | N= 1000 | N= 10000
------------------|---------|--------|---------
2.0 | 99.994 | x | x
1.5 | 95.73 | 99.996 | x
1.0 | 52.90 | 76.47 | 99.999
0.5 | 8.31 | 31.12 | 99.995

The memory footprint increases with N (but by a rather small factor). The computation cost also increases with N by a small factor.

We recommend setting the NumTopRanksPerStream constant in dss.h as desired.

## 6. Some caveats

1) We do not skew the linkage between PS_PARTKEY and PS_SUPPKEY: 

That is, each part has a fixed number of suppliers and the relationship between {PARTKEY, SUPPKEY} pairs is a fixed deterministic function.  We do not skew this relationship because rows of the LINEITEM table also have 
{L_PARTKEY, L_SUPPKEY} and the generators for the LINEITEM and PARTSUPP tables do not have any explicit coordination.

2) A complication in code that is worth calling out is that the set_state calls in coupled generators (e.g., LINEITEM and ORDERS) only advance the seeds that are specific to their tables.

## 7. Debugging/ Understanding
Examine the `zipf_debug.log` for more details on what the new changes do.

## 8. Acknowledgments
Manoj Syamala helped with a code review. Vivek Narasayya wrote the [previous version](https://www.microsoft.com/en-us/download/details.aspx?id=52430) and also helped brainstorm these changes.