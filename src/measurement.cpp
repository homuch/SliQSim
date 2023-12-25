#include "Simulator.h"
#include "util_sim.h"
#include <gmpxx.h>

/**Function*************************************************************

  Synopsis    [measure a specific qubit and collapse to a specific state]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Simulator::measure_and_collapse(std::unordered_map<int, int> &qubit_to_state){
    double oneroot2 = 1 / sqrt(2);
    double H_factor = pow(oneroot2, k%2);
    int nAnci_oneInt = ceil(log(r) / log(2)), nAnci_fourInt = ceil(log(w) / log(2));
    int nVar = n + nAnci_oneInt + nAnci_fourInt;
    build_bigBDD(nAnci_oneInt, nAnci_fourInt);
    // move measured qubits to the top
    int *permutation = new int[nVar];
    int indCount1 = 0;
    int indCount0 = qubit_to_state.size();  // number of measured qubits
    int first_index;
    for (int i = 0; i < n; i++)
    {
        int index = Cudd_ReadInvPerm(manager, i);
        if (qubit_to_state.find(index)!=qubit_to_state.end())
        {
            permutation[indCount1] = index;
            indCount1++;
        }
        else
        {
            permutation[indCount0] = index;
            indCount0++;
        }
    }
    for(int i=n;i<nVar;i++)
        permutation[i] = Cudd_ReadInvPerm(manager, i);
    
    int dum = Cudd_ShuffleHeap(manager, permutation);    
    nodecount();
    // first_index = Cudd_ReadPerm(manager, 0);
    // first_index = permutation[0];
    // std::cout<<"[";
    // for (int i = 0; i < n; i++)
    // {
    //     std::cout<<Cudd_ReadPerm(manager, i)<<",";
    // }
    // std::cout<<"]"<<std::endl;
    // calculate probability of the state
    int comple;
    int noNode_f = 0; //flag: 1 if the node we want to measure is reduced

	double p, p_next;
    double rand_num;
    int position_node=-1;
    double epsilon = 0.001;
    double then_edge, else_edge, probability;
    double re, im;
    DdNode *node, *tmp, *next_tmp;
    
    Cudd_Ref(bigBDD); // if this line is not added, there might be some error in CUDD.

    /* traverse until the qubit measured */
    comple = Cudd_IsComplement(bigBDD);
    tmp = Cudd_Regular(bigBDD);
    std::unordered_map<int, int>::iterator it;
    p=1.0;
    while (it=qubit_to_state.find(tmp->index), it!=qubit_to_state.end())
    {
        std::cout<<'('<<it->first<<' '<<it->second<<')'<<std::endl;
        std::cout<<p<<std::endl;
        node = Cudd_NotCond(tmp, comple);
        Cudd_Ref(node);
        // std::cout<<measure_probability(tmp, k/2, nVar,nAnci_fourInt, it->second)<<std::endl;
        // std::cout<<measure_probability(tmp, k/2, nVar,nAnci_fourInt, !it->second)<<std::endl;
        
        p_next = measure_probability(node, k/2, nVar,nAnci_fourInt, it->second) * H_factor * H_factor * normalize_factor * normalize_factor;
        std::cout<<"p_next="<<p_next<<std::endl;
        // rus_normalize_factor /= sqrt(p_next);
        // p*=p_next;
        p=p_next;
        next_tmp = Cudd_Child(manager, tmp, it->second);
        Cudd_RecursiveDeref(manager, node);
        if (cuddIsConstant(next_tmp))
        {
            noNode_f = 1;
            std::cout<<"noNode_f"<<std::endl;
            // break;

        }
        if (it->second == 1)
        {
            tmp = cuddT(tmp);
        }
        else
        {
            comple ^= Cudd_IsComplement(cuddE(tmp));
            tmp = Cudd_Regular(cuddE(tmp));
        }
    }
    rus_normalize_factor /= sqrt(p);
    std::cout<<"final p="<<p<<std::endl;
    std::cout<<"final rus_normalize_factor="<<rus_normalize_factor<<std::endl;
    tmp = Cudd_ReadOne(manager);
    Cudd_Ref(tmp);
    DdNode *tmp2;
    for(qubit_to_state.begin();it!=qubit_to_state.end();it++){
        if (it->second)
            tmp2 = Cudd_bddAnd(manager, Cudd_bddIthVar(manager, it->first), tmp);
        else
            tmp2 = Cudd_bddAnd(manager, Cudd_Not(Cudd_bddIthVar(manager, it->first)), tmp);
        Cudd_Ref(tmp2);
        Cudd_RecursiveDeref(manager, tmp);
        tmp = tmp2;
    }

    int* assigned = new int[n];
    for(int i=0;i<w;i++){
        std::string b1, b2, b3, b4;
        for(int j=0;j<r;j++){
            assigned[0] = 0;
            assigned[1] = 0;

            b1 = std::to_string(!Cudd_IsComplement(Cudd_Eval(manager, All_Bdd[i][j], assigned)))+b1;
            assigned[1]=1;
            b2 = std::to_string(!Cudd_IsComplement(Cudd_Eval(manager, All_Bdd[i][j], assigned)))+b2;

            assigned[0]=1;
            assigned[1]=0;
            b3 = std::to_string(!Cudd_IsComplement(Cudd_Eval(manager, All_Bdd[i][j], assigned)))+b3;
            assigned[1]=1;
            b4 = std::to_string(!Cudd_IsComplement(Cudd_Eval(manager, All_Bdd[i][j], assigned)))+b4;
        }
        std::cout<<i<<'/'<<w<<std::endl;
        std::cout<<b1<<std::endl;
        std::cout<<b2<<std::endl;
        std::cout<<b3<<std::endl;
        std::cout<<b4<<std::endl;
    }

    for(int i=0;i<w;i++){
        for(int j=0;j<r;j++){
            tmp2 = All_Bdd[i][j];
            // std::cout<<"hellp"<<std::endl;
            for(it=qubit_to_state.begin();it!=qubit_to_state.end();it++){
                // std::cout<<it->first<<' '<<it->second<<Cudd_IsComplement(Cudd_bddIthVar(manager, it->first))<<std::endl;
                if(it->second ^ Cudd_IsComplement(Cudd_bddIthVar(manager, it->first)))
                    tmp = Cudd_Cofactor(manager, tmp2, Cudd_bddIthVar(manager, it->first));
                else
                    tmp = Cudd_Cofactor(manager, tmp2, Cudd_Not(Cudd_bddIthVar(manager, it->first)));
                Cudd_Ref(tmp);
                // reset to zero
                node = Cudd_bddAnd(manager, tmp, Cudd_NotCond(Cudd_bddIthVar(manager, it->first), !Cudd_IsComplement(Cudd_bddIthVar(manager, it->first))));
                Cudd_Ref(node);
                Cudd_RecursiveDeref(manager, tmp2);
                Cudd_RecursiveDeref(manager, tmp);
                tmp2 = node;
            }
            All_Bdd[i][j] = tmp2;
        }
    }
    Cudd_RecursiveDeref(manager, tmp);
    // Cudd_RecursiveDeref(manager, tmp);
    Cudd_RecursiveDeref(manager, bigBDD);
    // Cudd_RecursiveDeref(manager, bigBDD);
    // Cudd_RecursiveDeref(manager, bigBDD);

    // std::cout<<"update: "<<tmp->index<<std::endl;
    // Cudd_RecursiveDeref(manager, node);
    // std::cout<<noNode_f<<std::endl;
    // std::cout<<tmp->index<<std::endl;
    // //? is it correct?
    nodecount();
    std::cout<<"size"<<(manager->size)<<std::endl;

    for(int i=0;i<w;i++){
        std::string b1, b2, b3, b4;
        for(int j=0;j<r;j++){
            assigned[0] = 0;
            assigned[1] = 0;

            b1 = std::to_string(!Cudd_IsComplement(Cudd_Eval(manager, All_Bdd[i][j], assigned)))+b1;
            assigned[1]=1;
            b2 = std::to_string(!Cudd_IsComplement(Cudd_Eval(manager, All_Bdd[i][j], assigned)))+b2;

            assigned[0]=1;
            assigned[1]=0;
            b3 = std::to_string(!Cudd_IsComplement(Cudd_Eval(manager, All_Bdd[i][j], assigned)))+b3;
            assigned[1]=1;
            b4 = std::to_string(!Cudd_IsComplement(Cudd_Eval(manager, All_Bdd[i][j], assigned)))+b4;
        }
        std::cout<<i<<'/'<<w<<std::endl;
        std::cout<<b1<<std::endl;
        std::cout<<b2<<std::endl;
        std::cout<<b3<<std::endl;
        std::cout<<b4<<std::endl;
    }
        
    delete [] assigned;
    
    // node = (Cudd_NotCond(tmp, comple));
    // Cudd_Ref(node);

    // /* compute probability of the node */
    // if (noNode_f)
    // {
    //     p=1.0;
    // }
    // else
    // {
    //     node = (Cudd_NotCond(tmp, comple));
    //     Cudd_Ref(node);
    //     p = measure_probability(node, k/2, nVar, nAnci_fourInt, 0)+measure_probability(node, k/2, nVar, nAnci_fourInt, 1);
    //     p *= H_factor * H_factor * normalize_factor * normalize_factor;
        
    //     Cudd_RecursiveDeref(manager, node);
    //     if (abs(p - 1) > epsilon)
	// 	{
    //         std::cerr << "[error]Numerical error: p0 + p1 = " << p << ", not 1" << std::endl;
	// 		std::exit(1);
	// 	}
    // }
	
    // double error_tmp = abs(p0 + p1 - 1);
    // if (error_tmp > error)
	//     error = error_tmp;

    /* sample */
    
        // (*outcome)[index] = '1'; // LSB: qn-1
    // normalize_factor /= sqrt(p);
}


void Simulator::build_bigBDD(int nAnci_oneInt, int nAnci_fourInt){
    double oneroot2 = 1 / sqrt(2);
    double H_factor = pow(oneroot2, k%2);
    int nAnci = nAnci_oneInt + nAnci_fourInt, nnAnci_fourInt = n + nAnci_fourInt, nVar = n + nAnci;
    DdNode *tmp1, *tmp2, *tmp3;
    std::cout<<"k="<<k<<std::endl;
    if (isReorder) Cudd_AutodynDisable(manager);
    
    int *arrAnci_fourInt = new int[nAnci_fourInt];
    for (int i = 0; i < nAnci_fourInt; i++)
        arrAnci_fourInt[i] = 0;// a bit string
    int *arrAnci_oneInt = new int[nAnci_oneInt];
    for (int i = 0; i < nAnci_oneInt; i++)
        arrAnci_oneInt[i] = 0;// a bit string, record the index of bit(BDD of an integer)?
    
    bigBDD = Cudd_Not(Cudd_ReadOne(manager));
    Cudd_Ref(bigBDD);
    // * Note that Cudd_bddIthVar will automatically create new nodes if it does not exist.
    for (int i = 0; i < w; i++)// for all integers
    {
        tmp3 = Cudd_Not(Cudd_ReadOne(manager));
        Cudd_Ref(tmp3);
        for (int j = 0; j < r; j++)// for all bits
        {
            tmp1 = Cudd_ReadOne(manager);
            Cudd_Ref(tmp1);
            for (int h = n + nAnci - 1; h >= nnAnci_fourInt; h--)// add ancilla bits with value from arrAnci_fourInt, i.e., BDD's index
            {
                if (arrAnci_oneInt[h - nnAnci_fourInt])
                    tmp2 = Cudd_bddAnd(manager, Cudd_bddIthVar(manager, h), tmp1);
                else
                    tmp2 = Cudd_bddAnd(manager, Cudd_Not(Cudd_bddIthVar(manager, h)), tmp1);
                Cudd_Ref(tmp2);
                Cudd_RecursiveDeref(manager, tmp1);
                tmp1 = tmp2;
            }
            tmp2 = Cudd_bddAnd(manager, All_Bdd[i][j], tmp1);// AND with the target BDD, i.e., the BDD is (allowed to be) one when the index is assigned to it.
            Cudd_Ref(tmp2);
            Cudd_RecursiveDeref(manager, tmp1);
            // Cudd_RecursiveDeref(manager, All_Bdd[i][j]); // keep these BDDs for statevector after measurement
            tmp1 = tmp2;
            tmp2 = Cudd_bddOr(manager, tmp3, tmp1);// OR with this integer/id. I.e., the BDD is one when the var is valid index
            Cudd_Ref(tmp2);
            Cudd_RecursiveDeref(manager, tmp1);
            Cudd_RecursiveDeref(manager, tmp3);
            tmp3 = tmp2;
            full_adder_plus_1(nAnci_oneInt, arrAnci_oneInt);
        }
        tmp1 = Cudd_ReadOne(manager);
        Cudd_Ref(tmp1);
        // similarly, add ancilla bits with value from arrAnci_fourInt(integers' index)
        for (int j = nnAnci_fourInt - 1; j >= n; j--)
        {
            if (arrAnci_fourInt[j - n])
                tmp2 = Cudd_bddAnd(manager, Cudd_bddIthVar(manager, j), tmp1);
            else
                tmp2 = Cudd_bddAnd(manager, Cudd_Not(Cudd_bddIthVar(manager, j)), tmp1);
            Cudd_Ref(tmp2);
            Cudd_RecursiveDeref(manager, tmp1);
            tmp1 = tmp2;
        }
        tmp2 = Cudd_bddAnd(manager, tmp3, tmp1);// the BDD is True when valid BDD index and integer index
        Cudd_Ref(tmp2);
        Cudd_RecursiveDeref(manager, tmp1);
        Cudd_RecursiveDeref(manager, tmp3);
        tmp1 = tmp2;
        tmp2 = Cudd_bddOr(manager, bigBDD, tmp1);// ORed into the bigBDD
        Cudd_Ref(tmp2);
        Cudd_RecursiveDeref(manager, tmp1);
        Cudd_RecursiveDeref(manager, bigBDD);
        bigBDD = tmp2;
        full_adder_plus_1(nAnci_fourInt, arrAnci_fourInt);
        for (int j = 0; j < nAnci_oneInt; j++) // reset array: not necessary but straightforward
            arrAnci_oneInt[j] = 0;
    }
    nodecount();

    
    delete[] arrAnci_fourInt;
    delete[] arrAnci_oneInt;
}

/**Function*************************************************************

  Synopsis    [calculate probabilities for a single node(without creating bigBDD)]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
double Simulator::simple_measure(DdNode *node, int edge){

    double then_edge, else_edge, probability;
    double re, im;
    DdNode *child = Cudd_Child(manager, node, edge), *tmp;
    Cudd_Ref(child);
    int position_node = Cudd_ReadPerm(manager, Cudd_NodeReadIndex(node)), position_child = Cudd_ReadPerm(manager, Cudd_NodeReadIndex(child));
    int skip_level;
    std::unordered_map<DdNode *, double>::iterator it;
    it = Node_Table.find(child);

    /* deal with the nodes which are reduced */
    
    skip_level = position_child - position_node - 1;


    if (Cudd_IsConstant(child)) // constant
    {
        if (!(Cudd_IsComplement(child))) // constant 1(-1)
        {
            re = 0;
            im = 0;
            for (int i = 0; i < w; i++)
            {
                re -= cos(((double)i/w) * PI);
                im -= sin(((double)i/w) * PI);
            }
            probability = pow(re, 2) + pow(im, 2);
            Cudd_RecursiveDeref(manager, child);
            std::cout<<re<<' '<<im<<std::endl;
            std::cout<<"constant 1: "<<probability<<std::endl;
            return probability * pow(2, n - position_node - 1);
        }
        else // constant 0
        {
            Cudd_RecursiveDeref(manager, child);
            return 0;
        }
    }
    else
    {
        std::cout<<"position_node="<<position_node<<std::endl;
        std::cout<<"position_child="<<position_child<<std::endl;
        std::cout<<"n="<<n<<std::endl;
        if (true) // compute entry
        {
            double int_value;
            
            int oneEntry;
            re = 0;
            im = 0;
            /* TODO: BDD to truth table */
            for (int i = 0; i < w; i++) //compute each complex value
            {
                int_value = 0;
                for (int j = 0; j < r; j++) //compute each integer
                {
                    tmp = Cudd_bddAnd(manager, node, All_Bdd[i][j]);
                    Cudd_Ref(tmp);
                    child = Cudd_Child(manager, tmp, edge);
                    Cudd_Ref(child);
                    Cudd_RecursiveDeref(manager, tmp);
                    tmp = child;
                    oneEntry = !(Cudd_IsComplement(tmp));
                    Cudd_RecursiveDeref(manager, tmp);
                    if (j == r - 1)
                        int_value -= oneEntry * pow(2, j + shift - k/2);
                    else
                        int_value += oneEntry * pow(2, j + shift - k/2);
                }
                /* translate to re and im */
                re += int_value * cos((double) (w - i - 1)/w * PI);
                im += int_value * sin((double) (w - i - 1)/w * PI);
            }
            probability = pow(re, 2) + pow(im, 2);
        }
        else // trace edges
        {
            then_edge = simple_measure(child, 1);
            else_edge = simple_measure(child, 0);
            probability = then_edge + else_edge;
            std::cout<<"then_edge="<<then_edge<<std::endl;
            std::cout<<"else_edge="<<else_edge<<std::endl;
        }
    }

    Node_Table[child] = probability;
    Cudd_RecursiveDeref(manager, child);

    return probability * pow(2, skip_level);
}


/**Function*************************************************************

  Synopsis    [calculate probabilities for measurement]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
double Simulator::measure_probability(DdNode *node, int kd2, int nVar, int nAnci_fourInt, int edge)
{
    double then_edge, else_edge, probability;
    double re, im;
    DdNode *child = Cudd_Child(manager, node, edge);
    Cudd_Ref(child);
    int position_node = Cudd_ReadPerm(manager, Cudd_NodeReadIndex(node)), position_child = Cudd_ReadPerm(manager, Cudd_NodeReadIndex(child));
    int skip_level;
    std::unordered_map<DdNode *, double>::iterator it;
    it = Node_Table.find(child);

    /* deal with the nodes which are reduced */
    if (position_child > n)
        skip_level = n - position_node - 1;
    else
        skip_level = position_child - position_node - 1;

    /* compute probability of the node */
    if (it != Node_Table.end()) // node has been recorded
    {
        Cudd_RecursiveDeref(manager, child);
        return it->second * pow(2, skip_level);
    }
    else if (Cudd_IsConstant(child)) // constant
    {
        // std::cout<<"is constant"<<!(Cudd_IsComplement(child))<<std::endl;
        if (!(Cudd_IsComplement(child))) // constant 1
        {
            re = 0;
            im = 0;
            for (int i = 0; i < w; i++)
            {
                re -= cos(i/w * PI);
                im -= sin(i/w * PI);
            }
            probability = pow(re, 2) + pow(im, 2);
            Cudd_RecursiveDeref(manager, child);
            return probability * pow(2, n - position_node - 1);
        }
        else // constant 0
        {
            Cudd_RecursiveDeref(manager, child);
            return 0;
        }
    }
    else
    {
        if (position_child >= n) // compute entry
        {
            double int_value;
            DdNode *tmp;
            int oneEntry;
            re = 0;
            im = 0;
            int *assign = new int[nVar];
            for (int i = 0; i < nVar; i++)
                assign[i] = 0;
            /* TODO: BDD to truth table */
            for (int i = 0; i < w; i++) //compute each complex value
            {
                int_value = 0;
                std::string bitstring = "";
                for (int j = 0; j < r; j++) //compute each integer
                {
                    
                    tmp = Cudd_Eval(manager, child, assign);
                    Cudd_Ref(tmp);
                    oneEntry = !(Cudd_IsComplement(tmp));
                    bitstring = std::to_string(oneEntry)+bitstring;
                    Cudd_RecursiveDeref(manager, tmp);
                    if (j == r - 1)
                        int_value -= oneEntry * pow(2, j + shift - kd2);
                    else
                        int_value += oneEntry * pow(2, j + shift - kd2);
                    full_adder_plus_1_start(nVar, assign, n + nAnci_fourInt);
                }
                std::cout<<bitstring<<std::endl;
                std::cout<<int_value<<std::endl;
                /* translate to re and im */
                re += int_value * cos((double) (w - i - 1)/w * PI);
                im += int_value * sin((double) (w - i - 1)/w * PI);
                full_adder_plus_1_start(nVar, assign, n);
                for (int j = n + nAnci_fourInt; j < nVar; j++) // reset array: not necessary but straightforward
                    assign[j] = 0;
            }
            probability = pow(re, 2) + pow(im, 2);
            delete[] assign;
        }
        else // trace edges
        {
            std::cout<<"trace edge "<<Cudd_NodeReadIndex(child)<<std::endl;
            then_edge = measure_probability(child, kd2, nVar, nAnci_fourInt, 1);
            else_edge = measure_probability(child, kd2, nVar, nAnci_fourInt, 0);
            probability = then_edge + else_edge;
        }
    }

    Node_Table[child] = probability;
    Cudd_RecursiveDeref(manager, child);

    return probability * pow(2, skip_level);
}

/**Function*************************************************************

  Synopsis    [measure one qubit]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Simulator::measure_one(int index, int kd2, double H_factor, int nVar, int nAnci_fourInt, std::string *outcome)
{
    int comple;
    int noNode_f = 0; //flag: 1 if the node we want to measure is reduced
    std::uniform_real_distribution<double> dis(0.0, 1.0);
	  double p0, p1, p;
    double rand_num;
    double epsilon = 0.001;
    DdNode *node, *tmp;
    Cudd_Ref(bigBDD); // if this line is not added, there might be some error in CUDD.

    /* traverse until the qubit measured */
    comple = Cudd_IsComplement(bigBDD);
    tmp = Cudd_Regular(bigBDD);
    while (!(tmp->index == index))
    {
        if (cuddIsConstant(tmp))
        {
            noNode_f = 1;
            break;
        }
        if ((tmp->index < n) && (outcome->substr(n - 1 - tmp->index, 1) == "1"))
        {
            tmp = cuddT(tmp);
        }
        else
        {
            comple ^= Cudd_IsComplement(cuddE(tmp));
            tmp = Cudd_Regular(cuddE(tmp));
        }
    }

    if (noNode_f)
    {
        p0 = 0.5;
        p1 = 0.5;
    }
    else
    {
        node = (Cudd_NotCond(tmp, comple));
        Cudd_Ref(node);
        p0 = measure_probability(node, kd2, nVar, nAnci_fourInt, 0);
        p0 *= H_factor * H_factor * normalize_factor * normalize_factor * rus_normalize_factor * rus_normalize_factor;
        p1 = measure_probability(node, kd2, nVar, nAnci_fourInt, 1);
        p1 *= H_factor * H_factor * normalize_factor * normalize_factor * rus_normalize_factor * rus_normalize_factor;
        Cudd_RecursiveDeref(manager, node);
        p = p0 + p1;
        if (abs(p - 1) > epsilon)
		{
            std::cerr << "[error]Numerical error: p0 + p1 = " << p << ", not 1" << std::endl;
            std::cerr << "rus_normalize_factor="<<rus_normalize_factor<<std::endl;
			std::exit(1);
		}
    }
	
    double error_tmp = abs(p0 + p1 - 1);
    if (error_tmp > error)
	    error = error_tmp;

    /* sample */
    rand_num = dis(gen);
    if (rand_num > p0)
    {
        (*outcome)[n - 1 - index] = '1'; // LSB: q0
        // (*outcome)[index] = '1'; // LSB: qn-1
        normalize_factor /= sqrt(p1);
    }
    else
    {
        // (*outcome)[n - 1 - index] = '0'; // LSB: q0
        // (*outcome)[index] = '0'; // LSB: qn-1
        normalize_factor /= sqrt(p0);
    }
}

/**Function*************************************************************

  Synopsis    [measurement]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Simulator::measurement()
{
    double oneroot2 = 1 / sqrt(2);
    double H_factor = pow(oneroot2, k%2);
    int nAnci_oneInt = ceil(log(r) / log(2)), nAnci_fourInt = ceil(log(w) / log(2)), nAnci = nAnci_oneInt + nAnci_fourInt, nnAnci_fourInt = n + nAnci_fourInt, nVar = n + nAnci;
    DdNode *tmp1, *tmp2, *tmp3;

    if (isReorder) Cudd_AutodynDisable(manager);
    
    int *arrAnci_fourInt = new int[nAnci_fourInt];
    for (int i = 0; i < nAnci_fourInt; i++)
        arrAnci_fourInt[i] = 0;// a bit string
    int *arrAnci_oneInt = new int[nAnci_oneInt];
    for (int i = 0; i < nAnci_oneInt; i++)
        arrAnci_oneInt[i] = 0;// a bit string, record the index of bit(BDD of an integer)?
    
    bigBDD = Cudd_Not(Cudd_ReadOne(manager));
    Cudd_Ref(bigBDD);
    // * Note that Cudd_bddIthVar will automatically create new nodes if it does not exist.
    for (int i = 0; i < w; i++)// for all integers
    {
        tmp3 = Cudd_Not(Cudd_ReadOne(manager));
        Cudd_Ref(tmp3);
        for (int j = 0; j < r; j++)// for all bits
        {
            tmp1 = Cudd_ReadOne(manager);
            Cudd_Ref(tmp1);
            for (int h = n + nAnci - 1; h >= nnAnci_fourInt; h--)// add ancilla bits with value from arrAnci_fourInt, i.e., BDD's index
            {
                if (arrAnci_oneInt[h - nnAnci_fourInt])
                    tmp2 = Cudd_bddAnd(manager, Cudd_bddIthVar(manager, h), tmp1);
                else
                    tmp2 = Cudd_bddAnd(manager, Cudd_Not(Cudd_bddIthVar(manager, h)), tmp1);
                Cudd_Ref(tmp2);
                Cudd_RecursiveDeref(manager, tmp1);
                tmp1 = tmp2;
            }
            tmp2 = Cudd_bddAnd(manager, All_Bdd[i][j], tmp1);// AND with the target BDD, i.e., the BDD is (allowed to be) one when the index is assigned to it.
            Cudd_Ref(tmp2);
            Cudd_RecursiveDeref(manager, tmp1);
            // Cudd_RecursiveDeref(manager, All_Bdd[i][j]); // keep these BDDs for statevector after measurement
            tmp1 = tmp2;
            tmp2 = Cudd_bddOr(manager, tmp3, tmp1);// OR with this integer/id. I.e., the BDD is one when the var is valid index
            Cudd_Ref(tmp2);
            Cudd_RecursiveDeref(manager, tmp1);
            Cudd_RecursiveDeref(manager, tmp3);
            tmp3 = tmp2;
            full_adder_plus_1(nAnci_oneInt, arrAnci_oneInt);
        }
        tmp1 = Cudd_ReadOne(manager);
        Cudd_Ref(tmp1);
        // similarly, add ancilla bits with value from arrAnci_fourInt(integers' index)
        for (int j = nnAnci_fourInt - 1; j >= n; j--)
        {
            if (arrAnci_fourInt[j - n])
                tmp2 = Cudd_bddAnd(manager, Cudd_bddIthVar(manager, j), tmp1);
            else
                tmp2 = Cudd_bddAnd(manager, Cudd_Not(Cudd_bddIthVar(manager, j)), tmp1);
            Cudd_Ref(tmp2);
            Cudd_RecursiveDeref(manager, tmp1);
            tmp1 = tmp2;
        }
        tmp2 = Cudd_bddAnd(manager, tmp3, tmp1);// the BDD is True when valid BDD index and integer index
        Cudd_Ref(tmp2);
        Cudd_RecursiveDeref(manager, tmp1);
        Cudd_RecursiveDeref(manager, tmp3);
        tmp1 = tmp2;
        tmp2 = Cudd_bddOr(manager, bigBDD, tmp1);// ORed into the bigBDD
        Cudd_Ref(tmp2);
        Cudd_RecursiveDeref(manager, tmp1);
        Cudd_RecursiveDeref(manager, bigBDD);
        bigBDD = tmp2;
        full_adder_plus_1(nAnci_fourInt, arrAnci_fourInt);
        for (int j = 0; j < nAnci_oneInt; j++) // reset array: not necessary but straightforward
            arrAnci_oneInt[j] = 0;
    }
    nodecount();

    // move measured qubits to the top
    int *permutation = new int[nVar];
    int indCount1 = 0;
    int indCount0 = 0;  // number of measured qubits
    for (int i = 0; i < n; i++){
        if (!measured_qubits_to_clbits[i].empty())
        {
            indCount0++;
        }
    }
    for (int i = 0; i < n; i++)
    {
        int index = Cudd_ReadInvPerm(manager, i);
        if (!measured_qubits_to_clbits[index].empty())
        {
            permutation[indCount1] = index;
            indCount1++;
        }
        else
        {
            permutation[indCount0] = index;
            indCount0++;
        }
    }
    for (int i = n; i < nVar; i++)
        permutation[i] = Cudd_ReadInvPerm(manager, i);
    int dum = Cudd_ShuffleHeap(manager, permutation);
    nodecount();

    std::unordered_map<std::string, int>::iterator it;
    std::string measure_outcome_qubits;
    std::string measure_outcome_clbits;
    for (int i = 0; i < shots; i++)
    {
        for (int j = 0; j < n; j++)
        {
            measure_outcome_qubits += '0';
        }
        for (int j = 0; j < nClbits; j++) 
        {
            measure_outcome_clbits += '0';
        }
        normalize_factor = 1;

        for (int j = 0; j < n; j++)
        {
            if (!measured_qubits_to_clbits[j].empty())
            {
                measure_one(j, k/2, H_factor, nVar, nAnci_fourInt, &measure_outcome_qubits);            
            }
        }
        
        // convert measurement outcome of qubits to clbits
        for (int qIndex = 0; qIndex < n; qIndex++)
        {
            for (int cIndex : measured_qubits_to_clbits[qIndex])
            {
                measure_outcome_clbits[nClbits - 1 - cIndex] = measure_outcome_qubits[n - 1 - qIndex];
                // the order is reversed
            }
        }
        it = state_count.find(measure_outcome_clbits);
        if (it != state_count.end())
            state_count[measure_outcome_clbits] = it->second + 1;
        else
            state_count[measure_outcome_clbits] = 1;
        measure_outcome = measure_outcome_qubits;
        measure_outcome_qubits.clear();
        measure_outcome_clbits.clear();
    }

    Cudd_RecursiveDeref(manager, bigBDD);
    delete[] arrAnci_fourInt;
    delete[] arrAnci_oneInt;
    delete[] permutation;
}

/**Function*************************************************************

  Synopsis    [get statevector string based on BDDs]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Simulator::getStatevector()
{
    mpf_t one_over_sqrt_2;        
    mpf_init(one_over_sqrt_2);
    mpf_sqrt_ui(one_over_sqrt_2, 2);
    mpf_div_ui(one_over_sqrt_2, one_over_sqrt_2, 2);
    // sqrt_val = 1/sqrt(2)
    
    mpz_t int_value;
    mpz_init(int_value);    
    mpf_t int_value_as_float;
    mpf_init(int_value_as_float);
    
    mpf_t re, im;
    mpf_init(re);
    mpf_init(im);
    
    mpf_t cos_val;
    mpf_init(cos_val);
    mpf_t sin_val;
    mpf_init(sin_val);    
    
    mpz_t tmp_int;
    mpz_init(tmp_int);
    mpf_t tmp_float;
    mpf_init(tmp_float);
    
    std::string bitstring;
    
    mpz_t two;
    mpz_init(two);
    mpz_set_str(two, "2", 10);
    mpf_t H_factor;
    mpf_init(H_factor);
    mpz_pow_ui(tmp_int, two, k / 2);
    mpf_set_z(H_factor, tmp_int);    
    if (k % 2 == 1)
        mpf_div(H_factor, H_factor, one_over_sqrt_2);
    
    long double final_re = 0;
    long double final_im = 0;
    
    int *assign = new int[n];
    unsigned long long nEntries = pow(2, n);      // should timeout before overflow occurs in "nEntries"
    int oneEntry;
    DdNode *tmp;
    
    for (int i = 0; i < n; i++)                   //initialize assignment
        assign[i] = 0;

    statevector = "[";
    
    for (unsigned long long i = 0; i < nEntries; i++) // compute every entry
    {
        final_re = 0;
        final_im = 0;
        mpf_init(re);
        mpf_init(im);
        bool isZero = 0;
        for (int j = 0; j < n; j++)
        {
            if (!measured_qubits_to_clbits[j].empty())
                if (assign[j] != stoi(measure_outcome.substr(n - 1 - j, 1)))
                {
                    isZero = 1;
                    break;
                }
        }
        
        if (isZero == 0)
        {
            for (int j = 0; j < w; j++) // compute every complex value
            {
                bitstring = "";
                
                mpz_init(int_value);
                for (int h = 0; h < r; h++) // compute every integer
                {
                    tmp = Cudd_Eval(manager, All_Bdd[j][h], assign);
                    Cudd_Ref(tmp);
                    oneEntry = !(Cudd_IsComplement(tmp));
                    Cudd_RecursiveDeref(manager, tmp);
                    if (Cudd_IsComplement(tmp))
                    {
                        bitstring += "0";
                    }
                    else
                    {
                        bitstring += "1";
                    }
                    
                }
                std::reverse(bitstring.begin(), bitstring.end());
                bool isNeg = (bitstring[0]=='1');
                if (isNeg)
                {
                    std::string inv_str = "1";
                    for (int digitIndex = 0; digitIndex < r; digitIndex++)
                    {
                        inv_str += "0";
                    }
                    mpz_t raw_data;
                    mpz_init_set_str(raw_data, bitstring.c_str(), 2);
                    mpz_t inverter;
                    mpz_init_set_str(inverter, inv_str.c_str(), 2);
                    
                    mpz_sub(int_value, raw_data, inverter);
                }
                else
                {
                    mpz_set_str(int_value, bitstring.c_str(), 2);
                }
                
                switch (j)
                {
                case 3:
                    mpf_set_d(cos_val, 1);
                    mpf_set_d(sin_val, 0);
                    break;
                case 2:
                    mpf_set(cos_val, one_over_sqrt_2);
                    mpf_set(sin_val, one_over_sqrt_2);
                    break;
                case 1:
                    mpf_set_d(cos_val, 0);
                    mpf_set_d(sin_val, 1);
                    break;
                case 0:
                    mpf_neg(cos_val, one_over_sqrt_2); // -sqrt(2)/2
                    mpf_set(sin_val, one_over_sqrt_2);
                    break;
                default:
                    std::cerr << "Warning: Unknown index (" << j << ")\n";
                    break;
                }
                                
                mpf_set_z(int_value_as_float, int_value);
                
                mpf_mul(tmp_float, int_value_as_float, cos_val);
                mpf_add(re, re, tmp_float);
                mpf_mul(tmp_float, int_value_as_float, sin_val);
                mpf_add(im, im, tmp_float);
                // translate to re and im
            }
            
            mpf_div(tmp_float, re, H_factor);            
            final_re = mpf_get_d(tmp_float) * normalize_factor;
            mpf_div(tmp_float, im, H_factor);
            final_im = mpf_get_d(tmp_float) * normalize_factor;
        }

        if ((final_re == 0)&&(final_im == 0))
            statevector = statevector + "\"0\"";
        else if (final_re == 0)
            statevector = statevector + "\"" + std::to_string(final_im) + "i\"";
        else if (final_im == 0)
            statevector = statevector + "\"" + std::to_string(final_re) + "\"";
        else
        {
            if (final_im < 0)
                statevector = statevector + "\"" + std::to_string(final_re) + std::to_string(final_im) + "i\"";
            else
                statevector = statevector + "\"" + std::to_string(final_re) + "+" + std::to_string(final_im) + "i\"";
        }
        if (i != nEntries - 1)
            statevector = statevector + ", ";
        full_adder_plus_1(n, assign);
    }
    statevector += "]";

    delete[] assign;
}