template <typename T>
class singleton
{
private:
    //pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;
    singleton() {}
    singleton(const singleton &s) {}
    singleton & operator = (const singleton & s) {}

public:
    static T *getInstance()
    {
        return GC::pinstance;
    }

    class GC
    {
    public:
        static T *pinstance;
        GC()
        {
            pinstance = new T();
        }
        ~GC()
        {
            if( pinstance )
            {
    //            pthread_mutex_lock(&mutex_lock);
                    delete pinstance;
    //            pthread_mutex_unlock(&mutex_lock);
            }
        }
    };
    static GC gc;
};
template <typename T>
T * singleton<T>::GC::pinstance = nullptr;


