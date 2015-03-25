template <typename T>
class singleton
{
public:
    //pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;
    singleton() {}
    singleton(const singleton &s) {}
    singleton & operator = (const singleton & s) {}

	//static T *pinstance;

public:
    static T *getInstance()
    {
        return GC::pinstance;
        //return pinstance;
    }

    class GC
    {
    public:
        static T *pinstance;
        GC()
        {
            if(!pinstance) pinstance = new T();
        }
        ~GC()
        {
            if( pinstance )
            {
    //            pthread_mutex_lock(&mutex_lock);
                    delete pinstance;
					pinstance = nullptr;
    //            pthread_mutex_unlock(&mutex_lock);
            }
        }
    };
    GC gc; // do not use static here! otherwise the constructor will not be called.
};
template <typename T>
T * singleton<T>::GC::pinstance = nullptr;
//T * singleton<T>::pinstance = nullptr;


